#include "world/zone_story_quest_feature.h"
#include "world/zone_story_quest_story.h"

#include <cjson/cJSON.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>

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

std::string read(const char *path)
{
	std::ifstream input(path);
	require(static_cast<bool>(input), "cannot read fixture");
	return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

std::string text(const cJSON *object, const char *key)
{
	const auto *field = cJSON_GetObjectItemCaseSensitive(object, key);
	return cJSON_IsString(field) ? field->valuestring : "";
}

int number(const cJSON *object, const char *key)
{
	return cJSON_GetObjectItemCaseSensitive(object, key)->valueint;
}

bool flag(const cJSON *object, const char *key)
{
	return cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(object, key));
}

zone_story_quest_catalog::catalog native_catalog(const char *path)
{
	std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(read(path).c_str()),
							     cJSON_Delete);
	require(root != nullptr, "cannot parse production fixture");
	zone_story_quest_catalog::catalog catalog;
	catalog.content_revision = number(root.get(), "content_revision");
	for (auto *zone = cJSON_GetObjectItemCaseSensitive(root.get(), "zones")->child; zone;
	     zone = zone->next)
		catalog.zones.push_back({ number(zone, "zone_number"), text(zone, "name"),
					  text(zone, "source_area"), number(zone, "first_vnum"),
					  number(zone, "last_vnum"), flag(zone, "discoverable") });
	for (auto *entry = cJSON_GetObjectItemCaseSensitive(root.get(), "definitions")->child;
	     entry; entry = entry->next)
	{
		zone_story_quest_tracking::quest_definition definition;
		definition.definition_id = text(entry, "definition_id");
		definition.source_system = text(entry, "source_system");
		definition.zone_number = number(entry, "zone_number");
		definition.source_area = text(entry, "source_area");
		definition.giver_vnum = number(entry, "giver_vnum");
		definition.completion_key = text(entry, "completion_key");
		definition.active = flag(entry, "active");
		definition.eligible_for_zone_completion =
			flag(entry, "eligible_for_zone_completion");
		definition.repeatable = flag(entry, "repeatable");
		definition.content_revision = number(entry, "content_revision");
		definition.display_name = text(entry, "display_name");
		definition.zone_name = text(entry, "zone_name");
		definition.objective = text(entry, "objective");
		definition.daily_eligible = flag(entry, "daily_eligible");
		definition.daily_exclusion = text(entry, "daily_exclusion");
		catalog.definitions.push_back(std::move(definition));
	}
	return catalog;
}

zone_story_quest_feature::completion_event completion(const std::string &id, const char *txid,
						      int64_t at)
{
	zone_story_quest_feature::completion_event event;
	event.transaction = { .schema_version = 2,
			      .transaction_id = txid,
			      .quest_definition_id = id,
			      .zone_number = 135,
			      .direct_completer_pid = 42,
			      .credited_pids = { 42 },
			      .room_vnum = 13556,
			      .completed_at = at,
			      .season_id = 7,
			      .content_revision = 2,
			      .daily_policy_revision = 1,
			      .daily_credited_pids = { 42 } };
	event.character_name = "Alice";
	event.level = event.strongest_party_level = 10;
	event.racewar = 1;
	event.party_context_known = true;
	return event;
}
} // namespace

int main(int argc, char **argv)
{
	using namespace zone_story_quest_feature;
	require(argc >= 3, "expected catalog and story fixture paths");
	auto catalog = native_catalog(argv[1]);
	const auto raw_catalog = catalog;
	require(zone_story_quest_catalog::eligible_definition_count(catalog, 135, 2) == 84,
		"raw Twin Towers count changed");
	std::string error;
	if (argc > 3 && (std::string(argv[3]) == "boundary" || std::string(argv[3]) == "oversized"))
	{
		const bool accepted = std::string(argv[3]) == "boundary";
		auto loaded = catalog;
		require(zone_story_quest_story::apply(read(argv[2]), "twin_towers_forest", &catalog,
						      &error) == accepted,
			"native sidecar apply disagreed with the byte boundary");
		require(zone_story_quest_story::load(
				&loaded, &error,
				std::filesystem::path(argv[2]).parent_path().string()) == accepted,
			"native file loader disagreed with the byte boundary");
		for (const auto *candidate : { &catalog, &loaded })
			require(candidate->story_mappings.size() == (accepted ? 1 : 0) &&
					zone_story_quest_catalog::eligible_definition_count(
						*candidate, 135, 2) == (accepted ? 10 : 84),
				"sidecar boundary failure partially applied a mapping");
		require(accepted || !error.empty(), "oversized sidecar lacked a diagnostic");
		return 0;
	}
	if (argc > 3 && std::string(argv[3]) == "all")
	{
		std::unique_ptr<cJSON, decltype(&cJSON_Delete)> snapshot(
			cJSON_Parse(read(argv[1]).c_str()), cJSON_Delete);
		for (auto *mapping =
			     cJSON_GetObjectItemCaseSensitive(snapshot.get(), "story_mappings")
				     ->child;
		     mapping; mapping = mapping->next)
		{
			char *json = cJSON_PrintUnformatted(mapping);
			const bool ok = zone_story_quest_story::apply(
				json, text(mapping, "source_area"), &catalog, &error);
			cJSON_free(json);
			if (!ok)
				std::cerr << error << '\n';
			require(ok, "a starter/town mapping failed native validation");
		}
		service tracker(catalog);
		for (const auto &mapping : catalog.story_mappings)
		{
			const auto &zone =
				*std::find_if(catalog.zones.begin(), catalog.zones.end(),
					      [&](const auto &z)
					      { return z.source_area == mapping.source_area; });
			require(tracker.discover_zone(7, 42, zone.zone_number,
						      std::max(1, zone.first_vnum), 100,
						      "arrival") == result::applied,
				"mapped area did not unlock discovery");
			const auto unseen = tracker.render_journal(7, 42, zone.zone_number, 10, 1,
								   101, false, false);
			for (const auto &contact : mapping.contacts)
			{
				require(unseen.find("[Met] " + contact.name) == std::string::npos,
					"unseen NPC was listed");
				require(tracker.meet_npc(7, 42, contact.mob_vnum,
							 std::max(1, zone.first_vnum),
							 101) == result::applied,
					"mapped contact was not tracked");
			}
			const auto journal = tracker.render_journal(7, 42, zone.zone_number, 10, 1,
								    102, false, false);
			for (const auto &contact : mapping.contacts)
				require(journal.find("[Met] " + contact.name) != std::string::npos,
					"met NPC was missing");
		}
		require(catalog.story_mappings.size() == 66 &&
				tracker.summary_for(7, 42).total == 1688,
			"native story projection disagreed with the complete source audit");
		auto file_catalog = raw_catalog;
		require(zone_story_quest_story::load(
				&file_catalog, &error,
				std::filesystem::path(argv[2]).parent_path().string()) &&
				file_catalog.story_mappings.size() ==
					catalog.story_mappings.size() &&
				zone_story_quest_catalog::eligible_definition_count(file_catalog,
										    831, 2) == 90 &&
				zone_story_quest_catalog::eligible_definition_count(file_catalog,
										    352, 2) == 3 &&
				zone_story_quest_catalog::eligible_definition_count(file_catalog,
										    140, 2) == 3 &&
				zone_story_quest_catalog::eligible_definition_count(file_catalog,
										    281, 2) == 6 &&
				zone_story_quest_catalog::eligible_definition_count(file_catalog,
										    431, 2) == 19 &&
				zone_story_quest_catalog::eligible_definition_count(file_catalog,
										    760, 2) == 7 &&
				zone_story_quest_catalog::eligible_definition_count(
					file_catalog, 5000, 2) == 17 &&
				zone_story_quest_catalog::eligible_definition_count(file_catalog,
										    1325, 2) == 8 &&
				zone_story_quest_catalog::eligible_definition_count(file_catalog,
										    57, 2) == 3 &&
				zone_story_quest_catalog::eligible_definition_count(file_catalog,
										    666, 2) == 8 &&
				zone_story_quest_catalog::eligible_definition_count(file_catalog,
										    404, 2) == 9 &&
				zone_story_quest_catalog::eligible_definition_count(file_catalog,
										    660, 2) == 0 &&
				zone_story_quest_catalog::eligible_definition_count(file_catalog,
										    777, 2) == 5,
			"complete Alatorin/Newhaven/Faerie/Verspin/Ship Yards/Ultarium/Surface sidecars failed the native file loader");
		const auto story_for = [&](const char *area, const char *id) -> const auto &
		{
			const auto mapping = std::find_if(catalog.story_mappings.begin(),
							  catalog.story_mappings.end(),
							  [&](const auto &m)
							  { return m.source_area == area; });
			require(mapping != catalog.story_mappings.end(), "journey mapping missing");
			const auto story = std::find_if(
				mapping->stories.begin(), mapping->stories.end(),
				[&](const auto &s) {
					return s.id ==
					       std::string("zone-story:story:") + area + ":" + id;
				});
			require(story != mapping->stories.end(), "journey story missing");
			return *story;
		};
		const auto record = [&](service &journey, const std::string &id, const char *txid,
					int zone, int room)
		{
			auto event = completion(id, txid, 120);
			event.transaction.zone_number = zone;
			event.transaction.room_vnum = room;
			require(journey.record_completion(event) == result::applied,
				"source story receipt was rejected");
		};
		const auto &fish = story_for("newbie", "feed-the-ailing-family");
		service family(catalog);
		require(family.discover_zone(7, 42, 292, 29296, 100, "arrival") ==
					result::applied &&
				family.meet_npc(7, 42, 29266, 29296, 101) == result::applied,
			"family encounter failed");
		zone_story_quest_catalog::journal_inventory supplies;
		supplies.carried[293] = 1;
		supplies.carried[294] = 1;
		auto journal =
			family.render_journal(7, 42, 292, 10, 1, 102, false, false, &supplies);
		require(fish.contracts.size() == 78 &&
				journal.find("[Ready now] " + fish.steps.front().text) !=
					std::string::npos &&
				journal.find("Next: " + fish.steps.back().text) !=
					std::string::npos,
			"mixed fish did not satisfy the two-fish delivery");
		supplies.carried.clear();
		supplies.carried[293] = 2;
		require(family.render_journal(7, 42, 292, 10, 1, 103, false, false, &supplies)
					.find("[Ready now] " + fish.steps.front().text) !=
				std::string::npos,
			"two matching fish were not accepted by the live checklist");
		record(family, fish.contracts.front(), "family-same", 292, 29296);
		record(family, fish.contracts.back(), "family-other", 292, 29296);
		require(family.progress_for_zone(7, 42, 292).completed == 1 &&
				family.progress_for_zone(7, 42, 292).total == 22,
			"fish recipes inflated Ailvio story completion");
		service restored_family(catalog), raw_family(raw_catalog);
		const auto saved_family = family.serialize_state();
		require(restored_family.deserialize_state(saved_family, &error) &&
				raw_family.deserialize_state(saved_family, &error) &&
				restored_family.progress_for_zone(7, 42, 292).completed == 1 &&
				raw_family.progress_for_zone(7, 42, 292).completed == 2,
			"fish grouping lost original receipts across restart");
		service supplied_note(catalog);
		require(supplied_note.discover_zone(7, 42, 292, 29227, 100, "arrival") ==
					result::applied &&
				supplied_note.meet_npc(7, 42, 29238, 29227, 101) == result::applied,
			"cleric encounter failed");
		const auto &cleric = story_for("newbie", "request-29238-c2cf98d3f50e");
		supplies.carried.clear();
		supplies.carried[29287] = 1;
		const auto before_read = supplied_note.serialize_state();
		journal = supplied_note.render_journal(7, 42, 292, 10, 1, 102, false, false,
						       &supplies);
		require(journal.find("Next: " + cleric.steps.back().text) != std::string::npos &&
				supplied_note.serialize_state() == before_read,
			"a supplied note required replaying the cure or reading created history");
		record(supplied_note, cleric.contracts.front(), "supplied-note", 292, 29227);
		require(supplied_note.progress_for_zone(7, 42, 292).completed == 1,
			"a terminal note delivery fabricated predecessor completion");
		service mansion(catalog);
		require(mansion.discover_zone(7, 42, 13, 1300, 100, "arrival") == result::applied &&
				mansion.meet_npc(7, 42, 1314, 1300, 101) == result::applied &&
				mansion.meet_npc(7, 42, 1316, 1338, 102) == result::applied,
			"mansion encounters failed");
		const auto &rescue = story_for("braddistock", "release-slippers");
		const auto &lord = story_for("braddistock", "quiet-the-mansion");
		supplies.carried.clear();
		supplies.carried[1340] = 1;
		journal = mansion.render_journal(7, 42, 13, 10, 1, 103, false, false, &supplies);
		require(journal.find(rescue.title) != std::string::npos &&
				journal.find("Next: " + lord.steps.back().text) !=
					std::string::npos,
			"pet service was hidden or a supplied collar required the key route");
		record(mansion, rescue.contracts.front(), "pet-rescue", 13, 1338);
		require(mansion.progress_for_zone(7, 42, 13).completed == 0 &&
				mansion.progress_for_zone(7, 42, 13).total == 1,
			"pet preparation prematurely completed the mansion story");
		record(mansion, lord.contracts.front(), "lord-collar", 13, 1300);
		require(mansion.progress_for_zone(7, 42, 13).completed == 1,
			"collar delivery did not complete the mansion story");
		const auto &triad = story_for("breale", "finish-the-triad-mixture-2602");
		service supplied_triad(catalog);
		require(supplied_triad.discover_zone(7, 42, 26, 2654, 100, "arrival") ==
					result::applied &&
				supplied_triad.meet_npc(7, 42, 2602, 2654, 101) == result::applied,
			"Triad encounter failed");
		supplies.carried.clear();
		supplies.carried[2670] = 1;
		supplies.carried[2672] = 2;
		supplies.carried[2675] = 2;
		journal = supplied_triad.render_journal(7, 42, 26, 10, 1, 102, false, false,
							&supplies);
		require(journal.find("Next: " + triad.steps.back().text) != std::string::npos &&
				supplied_triad.progress_for_zone(7, 42, 26).completed == 0,
			"a supplied Triad recipe required earlier exchanges or invented history");
		supplies.carried[2675] = 1;
		require(supplied_triad.render_journal(7, 42, 26, 10, 1, 103, false, false,
						      &supplies)
					.find("Next: Carry 2 x a red mushroom") !=
				std::string::npos,
			"Triad duplicate ingredients did not require two actual mushrooms");
		record(supplied_triad, triad.contracts.front(), "supplied-triad", 26, 2654);
		require(supplied_triad.progress_for_zone(7, 42, 26).completed == 1 &&
				supplied_triad.progress_for_zone(7, 42, 26).total == 6,
			"Pontif delivery fabricated the other Triad or wounded-witch exchanges");
		const auto &drider = story_for("elvish", "release-the-cursed-drider-35800");
		const auto &statues = story_for("elvish", "prepare-the-two-statues-35800");
		const auto &combine = story_for("elvish", "combine-the-statues-35801");
		service homestead(catalog);
		require(homestead.discover_zone(7, 42, 358, 35828, 100, "arrival") ==
					result::applied &&
				homestead.meet_npc(7, 42, 35800, 35828, 101) == result::applied &&
				homestead.meet_npc(7, 42, 35801, 35822, 102) == result::applied,
			"Homestead encounters failed");
		supplies.carried.clear();
		supplies.carried[35813] = 1;
		const auto homestead_before_read = homestead.serialize_state();
		journal = homestead.render_journal(7, 42, 358, 10, 1, 103, false, false, &supplies);
		require(journal.find("Next: " + drider.steps.back().text) != std::string::npos &&
				homestead.serialize_state() == homestead_before_read,
			"a supplied remedy required local egg/access/recipe history or read wrote state");
		record(homestead, statues.contracts.front(), "drider-prep", 358, 35828);
		record(homestead, combine.contracts.front(), "hermit-prep", 358, 35822);
		require(homestead.progress_for_zone(7, 42, 358).completed == 0 &&
				homestead.progress_for_zone(7, 42, 358).total == 2,
			"statue preparation prematurely completed the drider or egg request");
		record(homestead, drider.contracts.front(), "supplied-remedy", 358, 35828);
		service restored_homestead(catalog);
		require(restored_homestead.deserialize_state(homestead.serialize_state(), &error) &&
				restored_homestead.progress_for_zone(7, 42, 358).completed == 1,
			"drider terminal receipt did not survive restart independently of the egg request");
		const auto &household = story_for("krimman", "release-the-haunted-family-16422");
		const auto &fragment = story_for("krimman", "prepare-a-staff-fragment-16422");
		const auto &serving_girl = story_for("krimman", "release-the-serving-girl-16420");
		service supplied_keepsakes(catalog);
		require(supplied_keepsakes.discover_zone(7, 42, 164, 16459, 100, "arrival") ==
					result::applied &&
				supplied_keepsakes.meet_npc(7, 42, 16422, 16459, 101) ==
					result::applied,
			"haunted family encounter failed");
		supplies.carried.clear();
		for (const auto item : { 16452, 16453, 16454 })
			supplies.carried[item] = 1;
		const auto household_before_read = supplied_keepsakes.serialize_state();
		journal = supplied_keepsakes.render_journal(7, 42, 164, 10, 1, 102, false, false,
							    &supplies);
		require(journal.find("Next: " + household.steps.back().text) != std::string::npos &&
				supplied_keepsakes.serialize_state() == household_before_read,
			"supplied keepsakes required staff/rescue/access history or journal read wrote state");
		record(supplied_keepsakes, fragment.contracts.front(), "staff-service", 164, 16459);
		require(supplied_keepsakes.progress_for_zone(7, 42, 164).completed == 0 &&
				supplied_keepsakes.progress_for_zone(7, 42, 164).total == 8,
			"staff preparation became a rescue achievement");
		record(supplied_keepsakes, household.contracts.front(), "supplied-keepsakes", 164,
		       16459);
		require(supplied_keepsakes.progress_for_zone(7, 42, 164).completed == 1,
			"lord finale invented earlier family rescues");
		record(supplied_keepsakes, serving_girl.contracts.front(), "serving-girl-rescue",
		       164, 16450);
		require(supplied_keepsakes.progress_for_zone(7, 42, 164).completed == 2,
			"fragment-only rescue was treated as a rejected offering");
		const auto &knighthood =
			story_for("bastine", "the-bastine-road-knight-of-the-bastine-order-7600");
		const auto &first_commission =
			story_for("bastine", "the-bastine-road-young-warrior-7600");
		const auto &victor = story_for("bastine", "the-apprentice-s-trust-7603");
		service supplied_wand(catalog);
		require(supplied_wand.discover_zone(7, 42, 76, 7626, 100, "arrival") ==
					result::applied &&
				supplied_wand.meet_npc(7, 42, 7600, 7626, 101) == result::applied,
			"Bastine commission encounter failed");
		supplies.carried.clear();
		supplies.carried[70970] = 1;
		journal = supplied_wand.render_journal(7, 42, 76, 10, 1, 102, false, false,
						       &supplies);
		require(journal.find("Next: " + knighthood.steps.back().text) !=
					std::string::npos &&
				supplied_wand.progress_for_zone(7, 42, 76).completed == 0,
			"supplied final wand required earlier promotion history");
		record(supplied_wand, first_commission.contracts.front(), "first-commission", 76,
		       7626);
		require(supplied_wand.progress_for_zone(7, 42, 76).completed == 1 &&
				supplied_wand.progress_for_zone(7, 42, 76).total == 14,
			"early commission completed the entire knighthood campaign");
		record(supplied_wand, knighthood.contracts.front(), "gifted-wand", 76, 7626);
		record(supplied_wand, victor.contracts.front(), "victor-trust", 76, 7620);
		service restored_household(catalog), restored_bastine(catalog);
		require(restored_household.deserialize_state(supplied_keepsakes.serialize_state(),
							     &error) &&
				restored_bastine.deserialize_state(supplied_wand.serialize_state(),
								   &error) &&
				restored_household.progress_for_zone(7, 42, 164).completed == 2 &&
				restored_bastine.progress_for_zone(7, 42, 76).completed == 3 &&
				restored_bastine.progress_for_zone(7, 42, 413).completed == 0,
			"independent receipts changed on restart or Victor trust fabricated Highway rescue");
		const auto &auriam = story_for("pineholl", "help-the-wounded-gold-dragon-16006");
		const auto &hunter =
			story_for("pineholl", "the-cruel-warrior-s-dragon-trophies-16005");
		service supplied_dragon_gear(catalog), supplied_dragon_trophies(catalog);
		require(supplied_dragon_gear.discover_zone(7, 42, 160, 16077, 100, "arrival") ==
					result::applied &&
				supplied_dragon_gear.meet_npc(7, 42, 16006, 16077, 101) ==
					result::applied &&
				supplied_dragon_trophies.discover_zone(
					7, 42, 160, 16081, 100, "arrival") == result::applied &&
				supplied_dragon_trophies.meet_npc(7, 42, 16005, 16081, 101) ==
					result::applied,
			"Pine Hollow dragon encounters failed");
		supplies.carried.clear();
		for (const auto item : { 16013, 16014, 16080 })
			supplies.carried[item] = 1;
		const auto before_dragon_read = supplied_dragon_gear.serialize_state();
		journal = supplied_dragon_gear.render_journal(7, 42, 160, 10, 1, 102, false, false,
							      &supplies);
		require(journal.find("Next: " + auriam.steps.back().text) != std::string::npos &&
				supplied_dragon_gear.serialize_state() == before_dragon_read,
			"supplied dragon equipment required source/kill/topic history or journal read wrote state");
		record(supplied_dragon_gear, auriam.contracts.front(), "supplied-dragon-gear", 160,
		       16077);
		supplies.carried.clear();
		for (const auto item : { 16015, 16016, 16076, 16077 })
			supplies.carried[item] = 1;
		journal = supplied_dragon_trophies.render_journal(7, 42, 160, 10, 1, 102, false,
								  false, &supplies);
		require(journal.find("Next: " + hunter.steps.back().text) != std::string::npos &&
				supplied_dragon_gear.progress_for_zone(7, 42, 160).completed == 1 &&
				supplied_dragon_gear.progress_for_zone(7, 42, 160).total == 7,
			"independent dragon requests inferred a required earlier branch or combined accomplishment");
		record(supplied_dragon_trophies, hunter.contracts.front(),
		       "supplied-dragon-trophies", 160, 16081);
		record(supplied_dragon_gear, hunter.contracts.front(), "second-independent-dragon",
		       160, 16081);
		const auto &coat = story_for("pineholl", "a-heavy-bear-coat-16080");
		const auto &jacket = story_for("pineholl", "a-brown-bear-jacket-16080");
		service supplied_skins(catalog);
		require(supplied_skins.discover_zone(7, 42, 160, 16110, 100, "arrival") ==
					result::applied &&
				supplied_skins.meet_npc(7, 42, 16080, 16110, 101) ==
					result::applied,
			"Darlene encounter failed");
		supplies.carried.clear();
		supplies.carried[16021] = 1;
		supplies.carried[16020] = 2;
		supplies.carried[16019] = 2;
		const auto before_skin_read = supplied_skins.serialize_state();
		journal = supplied_skins.render_journal(7, 42, 160, 10, 1, 102, false, false,
							&supplies);
		require(journal.find("Next: " + coat.steps.front().text) != std::string::npos &&
				journal.find("Next: " + coat.steps.back().text) ==
					std::string::npos &&
				journal.find("Next: " + jacket.steps.front().text) !=
					std::string::npos &&
				journal.find("Next: " + jacket.steps.back().text) ==
					std::string::npos &&
				supplied_skins.serialize_state() == before_skin_read,
			"one huge skin or two ordinary skins satisfied the exact coat/jacket recipe");
		supplies.carried[16021] = 2;
		supplies.carried[16019] = 3;
		journal = supplied_skins.render_journal(7, 42, 160, 10, 1, 102, false, false,
							&supplies);
		require(journal.find("Next: " + coat.steps.back().text) != std::string::npos &&
				journal.find("Next: " + jacket.steps.back().text) !=
					std::string::npos &&
				supplied_skins.progress_for_zone(7, 42, 160).completed == 0,
			"complete supplied skins required local hunting or created a delivery receipt");
		record(supplied_skins, coat.contracts.front(), "supplied-coat-skins", 160, 16110);
		service restored_dragons(catalog), restored_trophies(catalog),
			restored_skins(catalog);
		require(restored_dragons.deserialize_state(supplied_dragon_gear.serialize_state(),
							   &error) &&
				restored_trophies.deserialize_state(
					supplied_dragon_trophies.serialize_state(), &error) &&
				restored_skins.deserialize_state(supplied_skins.serialize_state(),
								 &error) &&
				restored_dragons.progress_for_zone(7, 42, 160).completed == 2 &&
				restored_trophies.progress_for_zone(7, 42, 160).completed == 1 &&
				restored_skins.progress_for_zone(7, 42, 160).completed == 1 &&
				restored_skins.progress_for_zone(7, 42, 40).completed == 0 &&
				restored_dragons.progress_for_zone(7, 42, 163).completed == 0,
			"Pine receipts changed on restart or fabricated other clothing/foreign accomplishments");
		const auto &drow_mission =
			story_for("quietus", "the-drow-lieutenant-s-contract-1734");
		const auto &orc_mission =
			story_for("quietus", "the-orcish-lieutenant-s-contract-1735");
		const auto &staff_mission =
			story_for("quietus", "the-angry-lieutenant-s-contract-1736");
		const auto &bloodstone_mission = story_for("quietus", "proof-against-quietus-1749");
		service supplied_quietus(catalog);
		require(supplied_quietus.discover_zone(7, 42, 17, 1758, 100, "arrival") ==
				result::applied,
			"Quietus discovery failed");
		const int officers[][2] = { { 1728, 1758 },
					    { 1734, 1768 },
					    { 1735, 1770 },
					    { 1736, 1782 },
					    { 1749, 1783 } };
		for (const auto &officer : officers)
			require(supplied_quietus.meet_npc(7, 42, officer[0], officer[1], 101) ==
					result::applied,
				"Quietus officer encounter failed");
		supplies.carried.clear();
		supplies.carried[1746] = 1;
		journal = supplied_quietus.render_journal(7, 42, 17, 10, 1, 102, false, false,
							  &supplies);
		require(journal.find("Next: " + drow_mission.steps[1].text) != std::string::npos &&
				journal.find("Next: " + drow_mission.steps.back().text) ==
					std::string::npos,
			"Aresliean's head alone satisfied the required note-and-head delivery");
		supplies.carried[1732] = 1;
		supplies.carried[9436] = 1;
		supplies.carried[16450] = 1;
		supplies.carried[1730] = 1;
		const auto before_quietus_read = supplied_quietus.serialize_state();
		journal = supplied_quietus.render_journal(7, 42, 17, 10, 1, 102, false, false,
							  &supplies);
		for (const auto *mission :
		     { &drow_mission, &orc_mission, &staff_mission, &bloodstone_mission })
		{
			require(journal.find("Next: " + mission->steps.back().text) !=
					std::string::npos,
				"supplied Quietus proof required optional briefing, membership or personal kill history");
			for (const auto &step : mission->steps)
				if (step.optional)
					require(journal.find("Next: " + step.text) ==
							std::string::npos,
						"optional Quietus briefing displaced required delivery");
		}
		require(supplied_quietus.serialize_state() == before_quietus_read &&
				supplied_quietus.progress_for_zone(7, 42, 17).total == 4 &&
				supplied_quietus.progress_for_zone(7, 42, 17).completed == 0,
			"Quietus read/possession fabricated mission completion or counted support services");
		const auto &credential_intro =
			story_for("quietus", "show-membership-credentials-1728");
		const auto &seal_briefing = story_for("quietus", "show-the-captain-a-seal-1749");
		const auto &dagger_briefing =
			story_for("quietus", "show-the-captain-a-mission-reward-1749");
		record(supplied_quietus, credential_intro.contracts[0], "badge-intro", 17, 1758);
		record(supplied_quietus, credential_intro.contracts[1], "longsword-intro", 17,
		       1758);
		record(supplied_quietus, seal_briefing.contracts[0], "seal-briefing", 17, 1783);
		record(supplied_quietus, dagger_briefing.contracts[0], "dagger-briefing", 17, 1783);
		require(supplied_quietus.progress_for_zone(7, 42, 17).completed == 0,
			"credential/proof briefings were counted as completed missions");
		for (const auto *mission :
		     { &drow_mission, &orc_mission, &staff_mission, &bloodstone_mission })
			record(supplied_quietus, mission->contracts.front(), mission->id.c_str(),
			       17, 1770);
		service restored_quietus(catalog);
		require(restored_quietus.deserialize_state(supplied_quietus.serialize_state(),
							   &error) &&
				restored_quietus.progress_for_zone(7, 42, 17).completed == 4 &&
				restored_quietus.progress_for_zone(7, 42, 17).total == 4 &&
				restored_quietus.progress_for_zone(7, 42, 94).completed == 0 &&
				restored_quietus.progress_for_zone(7, 42, 164).completed == 0,
			"Quietus reload changed mission receipts or invented foreign source accomplishments");
		const auto &hide_commission = story_for("torg", "a-dracolich-hide-bracelet-28936");
		const auto &rose_commission = story_for("torg", "a-secret-rose-delivery-28937");
		const auto &buckle_commission = story_for("torg", "an-obsidian-buckle-29024");
		const auto &ring_commission =
			story_for("torg", "evidence-of-a-secret-affair-28932");
		const auto &legend_commission =
			story_for("torg", "relics-of-the-eight-legends-28964");
		const auto &fine_chisel_commission =
			story_for("torg", "a-fine-chisel-for-the-craftsman");
		service supplied_torg(catalog);
		require(supplied_torg.discover_zone(7, 42, 289, 28900, 100, "arrival") ==
				result::applied,
			"Torg discovery failed");
		for (int contact : { 28917, 28922, 28929, 28932, 28936, 28937, 28964, 28965, 28974,
				     29023, 29024 })
			require(supplied_torg.meet_npc(7, 42, contact, 28900, 101) ==
					result::applied,
				"Torg contact encounter failed");
		supplies.carried.clear();
		supplies.carried[28924] = 1;
		supplies.carried[28955] = 1;
		supplies.carried[28962] = 1;
		supplies.carried[28982] = 1;
		supplies.carried[28916] = 2;
		supplies.carried[28944] = 8;
		const auto before_torg_read = supplied_torg.serialize_state();
		journal = supplied_torg.render_journal(7, 42, 289, 10, 1, 102, false, false,
						       &supplies);
		for (const auto *commission :
		     { &hide_commission, &rose_commission, &buckle_commission })
		{
			require(journal.find("Next: " + commission->steps.back().text) !=
					std::string::npos,
				"supplied Torg materials required optional preparation or personal source history");
			for (const auto &step : commission->steps)
				if (step.optional)
					require(journal.find("Next: " + step.text) ==
							std::string::npos,
						"optional Torg preparation displaced required delivery");
		}
		require(journal.find("Next: " + ring_commission.steps[1].text) !=
					std::string::npos &&
				journal.find("Next: " + ring_commission.steps.back().text) ==
					std::string::npos,
			"two copies of one promise ring substituted for two distinct ring kinds");
		require(journal.find("Next: " + legend_commission.steps[1].text) !=
					std::string::npos &&
				journal.find("Next: " + legend_commission.steps.back().text) ==
					std::string::npos,
			"eight copies of one relic substituted for all eight distinct legends");
		supplies.carried[28938] = 1;
		for (int relic = 28944; relic <= 28951; ++relic)
			supplies.carried[relic] = 1;
		journal = supplied_torg.render_journal(7, 42, 289, 10, 1, 102, false, false,
						       &supplies);
		require(journal.find("Next: " + ring_commission.steps.back().text) !=
					std::string::npos &&
				journal.find("Next: " + legend_commission.steps.back().text) !=
					std::string::npos &&
				supplied_torg.serialize_state() == before_torg_read &&
				supplied_torg.progress_for_zone(7, 42, 289).completed == 0 &&
				supplied_torg.progress_for_zone(7, 42, 289).total == 12,
			"Torg read/possession changed history or counted preparation services");
		for (const auto &id :
		     { "cure-dracolich-hide-28917", "prepare-a-secret-rose-28965" })
		{
			const auto &preparation = story_for("torg", id);
			record(supplied_torg, preparation.contracts.front(), id, 289, 28900);
		}
		require(supplied_torg.progress_for_zone(7, 42, 289).completed == 0,
			"Torg curing or rose service added an achievement");
		record(supplied_torg, fine_chisel_commission.contracts[0], "old-master", 289,
		       28900);
		record(supplied_torg, fine_chisel_commission.contracts[1], "arriving-master", 289,
		       28900);
		require(supplied_torg.progress_for_zone(7, 42, 289).completed == 1,
			"both fine-chisel alternatives counted as two achievements");
		for (const auto &mapping : catalog.story_mappings)
			if (mapping.source_area == "torg")
				for (const auto &commission : mapping.stories)
					if (commission.category != "service" &&
					    commission.id != fine_chisel_commission.id)
						record(supplied_torg, commission.contracts.front(),
						       commission.id.c_str(), 289, 28900);
		service restored_torg(catalog);
		require(restored_torg.deserialize_state(supplied_torg.serialize_state(), &error) &&
				restored_torg.progress_for_zone(7, 42, 289).completed == 12 &&
				restored_torg.progress_for_zone(7, 42, 289).total == 12 &&
				restored_torg.progress_for_zone(7, 42, 550).completed == 0 &&
				restored_torg.progress_for_zone(7, 42, 875).completed == 0 &&
				restored_torg.progress_for_zone(7, 42, 712).completed == 0,
			"Torg reload changed local receipts or fabricated foreign quest completion");
		const auto &grove_robes = story_for("solonar", "robes-of-the-arch-magi-30603");
		const auto &grove_cloak = story_for("solonar", "a-piwafwi-of-power-30604");
		const auto &mage_bane = story_for("solonar", "forge-mage-bane-30638");
		const auto &lich_scroll = story_for("solonar", "prepare-an-ancient-scroll-30617");
		service supplied_grove(catalog);
		require(supplied_grove.discover_zone(7, 42, 306, 30648, 100, "arrival") ==
				result::applied,
			"grove arrival discovery failed");
		for (int contact : { 30600, 30601, 30602, 30603, 30604, 30605, 30606, 30615, 30617,
				     30620, 30631, 30632, 30635, 30638 })
			require(supplied_grove.meet_npc(7, 42, contact, 30648, 101) ==
					result::applied,
				"grove fixture encounter failed");
		supplies.carried.clear();
		for (int material : { 30634, 30636, 30639, 30640, 30641, 30642, 30649, 30650, 30656,
				      30657, 30658, 30662, 30664, 30666 })
			supplies.carried[material] = 1;
		const auto before_grove_read = supplied_grove.serialize_state();
		journal = supplied_grove.render_journal(7, 42, 306, 10, 1, 102, false, false,
							&supplies);
		for (const auto *recipe : { &grove_robes, &lich_scroll })
			require(journal.find("Next: " + recipe->steps.back().text) !=
					std::string::npos,
				"supplied grove ingredients required optional crafting history");
		for (const auto *recipe : { &grove_robes, &grove_cloak, &mage_bane, &lich_scroll })
			for (const auto &step : recipe->steps)
				if (step.optional)
					require(journal.find("Next: " + step.text) ==
							std::string::npos,
						"optional grove preparation displaced current delivery");
		require(journal.find("Next: " + mage_bane.steps[2].text) != std::string::npos &&
				journal.find("Next: " + grove_cloak.steps[3].text) !=
					std::string::npos,
			"diamond/heart omitted pickaxe or lavender replaced raw golden thread");
		const std::string unavailable = "Turn-in currently unavailable:";
		size_t fee_warnings = 0;
		for (size_t at = journal.find(unavailable); at != std::string::npos;
		     at = journal.find(unavailable, at + unavailable.size()))
			++fee_warnings;
		require(fee_warnings == 3,
			"grove mixed fees were advertised as available or coin rewards became fees");
		supplies.carried[30659] = 1;
		supplies.carried[30635] = 1;
		journal = supplied_grove.render_journal(7, 42, 306, 10, 1, 102, false, false,
							&supplies);
		for (const auto *recipe : { &mage_bane, &grove_cloak })
			require(journal.find("Next: " + recipe->steps.back().text) !=
					std::string::npos,
				"exact supplied pickaxe/raw thread did not satisfy live ingredient checks");
		require(supplied_grove.serialize_state() == before_grove_read &&
				supplied_grove.progress_for_zone(7, 42, 306).completed == 0 &&
				supplied_grove.progress_for_zone(7, 42, 306).total == 5,
			"grove possession or reading created history or counted services");
		// Project recovered native receipts; this does not execute unavailable coin fees.
		for (const auto &mapping : catalog.story_mappings)
			if (mapping.source_area == "solonar")
				for (const auto &recipe : mapping.stories)
					if (recipe.category == "service")
						record(supplied_grove, recipe.contracts.front(),
						       recipe.id.c_str(), 306, 30648);
		require(supplied_grove.progress_for_zone(7, 42, 306).completed == 0,
			"grove intermediate recipes added achievements");
		supplies.carried.erase(30640);
		supplies.carried.erase(30635);
		journal = supplied_grove.render_journal(7, 42, 306, 10, 1, 122, false, false,
							&supplies);
		require(journal.find("Next: " + grove_robes.steps[5].text) != std::string::npos &&
				journal.find("Next: " + grove_cloak.steps[3].text) !=
					std::string::npos &&
				journal.find("Next: " + grove_robes.steps.back().text) ==
					std::string::npos &&
				journal.find("Next: " + grove_cloak.steps.back().text) ==
					std::string::npos,
			"recorded grove preparation recreated consumed orb or raw-thread stock");
		for (const auto &mapping : catalog.story_mappings)
			if (mapping.source_area == "solonar")
				for (const auto &recipe : mapping.stories)
					if (recipe.category != "service")
						record(supplied_grove, recipe.contracts.front(),
						       recipe.id.c_str(), 306, 30648);
		service restored_grove(catalog);
		require(restored_grove.deserialize_state(supplied_grove.serialize_state(),
							 &error) &&
				restored_grove.progress_for_zone(7, 42, 306).completed == 5 &&
				restored_grove.progress_for_zone(7, 42, 306).total == 5 &&
				restored_grove.progress_for_zone(7, 42, 358).completed == 0 &&
				restored_grove.progress_for_zone(7, 42, 831).completed == 0,
			"grove reload changed local receipts or invented foreign completion");
		const auto &storm = story_for("wh", "request-55103-dd5688196e76");
		const auto &cosmos = story_for("wh", "request-55103-0e8b41819618");
		const auto &dagger_marks = story_for("wh", "request-55116-e78a927f5454");
		const auto &chief_key = story_for("wh", "request-55229-23f9768a6235");
		const auto &winter =
			*std::find_if(catalog.story_mappings.begin(), catalog.story_mappings.end(),
				      [](const auto &m) { return m.source_area == "wh"; });
		service supplied_winter(catalog);
		require(supplied_winter.discover_zone(7, 42, 550, 55125, 100, "arrival") ==
				result::applied,
			"Winterhaven arrival discovery failed");
		for (const auto &contact : winter.contacts)
			require(supplied_winter.meet_npc(7, 42, contact.mob_vnum, 55125, 101) ==
					result::applied,
				"Winterhaven fixture encounter failed");
		supplies.carried.clear();
		for (int material : { 22631, 34541, 55166, 55209, 55284, 75856, 76050, 76066, 76243,
				      76634, 82553, 82554, 82555, 55167, 55319, 55233 })
			supplies.carried[material] = 1;
		supplies.carried[55291] = 3;
		const auto before_winter_read = supplied_winter.serialize_state();
		journal = supplied_winter.render_journal(7, 42, 550, 10, 1, 102, false, false,
							 &supplies);
		for (const auto *recipe : { &storm, &chief_key })
			require(journal.find("Next: " + recipe->steps.back().text) !=
					std::string::npos,
				"supplied Winterhaven recipe required optional preparation");
		// The cosmos and storm share a giver, so check its distinct missing-dust text.
		require(journal.find("Next: " + cosmos.steps[5].text) != std::string::npos &&
				journal.find("Next: " + dagger_marks.steps[1].text) !=
					std::string::npos &&
				journal.find("Next: " + dagger_marks.steps.back().text) ==
					std::string::npos,
			"duplicate dust or a same-name dagger mark satisfied distinct requirements");
		for (const auto *recipe : { &storm, &cosmos, &chief_key })
			for (const auto &step : recipe->steps)
				if (step.optional)
					require(journal.find("Next: " + step.text) ==
							std::string::npos,
						"optional Winterhaven preparation displaced live requirements");
		fee_warnings = 0;
		for (size_t at = journal.find(unavailable); at != std::string::npos;
		     at = journal.find(unavailable, at + unavailable.size()))
			++fee_warnings;
		require(fee_warnings == 49,
			"Winterhaven fee refusals disappeared or item-only coin rewards became fees");
		supplies.carried[55319] = 2;
		supplies.carried[55375] = supplies.carried[55379] = 1;
		journal = supplied_winter.render_journal(7, 42, 550, 10, 1, 102, false, false,
							 &supplies);
		require(journal.find("Next: " + cosmos.steps[5].text) == std::string::npos &&
				journal.find("Next: " + dagger_marks.steps.back().text) !=
					std::string::npos &&
				supplied_winter.serialize_state() == before_winter_read &&
				supplied_winter.progress_for_zone(7, 42, 550).completed == 0 &&
				supplied_winter.progress_for_zone(7, 42, 550).total == 135,
			"Winterhaven exact stock failed or reading/possession created history");
		for (const auto &recipe : winter.stories)
			if (recipe.category == "service")
				record(supplied_winter, recipe.contracts.front(), recipe.id.c_str(),
				       550, 55125);
		require(supplied_winter.progress_for_zone(7, 42, 550).completed == 0,
			"Winterhaven services added achievements");
		supplies.carried.erase(55166);
		supplies.carried.erase(55209);
		journal = supplied_winter.render_journal(7, 42, 550, 10, 1, 122, false, false,
							 &supplies);
		require(journal.find("Next: " + storm.steps[4].text) != std::string::npos,
			"Winterhaven preparation receipts recreated consumed improved equipment");
		supplies.carried[55166] = 1;
		journal = supplied_winter.render_journal(7, 42, 550, 10, 1, 122, false, false,
							 &supplies);
		require(journal.find("Next: " + storm.steps[5].text) != std::string::npos,
			"Winterhaven orb receipt recreated consumed stock");
		const auto &first_memory = story_for("wh", "request-55202-7a73a52f2b9a");
		record(supplied_winter, first_memory.contracts.front(), "first-memory", 550, 55125);
		require(supplied_winter.progress_for_zone(7, 42, 550).completed == 1,
			"one memory receipt completed other independent deliveries");
		for (const auto &recipe : winter.stories)
			if (recipe.category != "service" && recipe.id != first_memory.id)
				record(supplied_winter, recipe.contracts.front(), recipe.id.c_str(),
				       550, 55125);
		service restored_winter(catalog);
		require(restored_winter.deserialize_state(supplied_winter.serialize_state(),
							  &error) &&
				restored_winter.progress_for_zone(7, 42, 550).completed == 135 &&
				restored_winter.progress_for_zone(7, 42, 550).total == 135 &&
				restored_winter.progress_for_zone(7, 42, 306).completed == 0 &&
				restored_winter.progress_for_zone(7, 42, 831).completed == 0,
			"Winterhaven reload changed local receipts or invented foreign completion");
		const auto &smoke =
			*std::find_if(catalog.story_mappings.begin(), catalog.story_mappings.end(),
				      [](const auto &m) { return m.source_area == "smokev"; });
		const auto &ivar_hearts = story_for("smokev", "the-two-dragon-hearts");
		const auto &ivar_talon = story_for("smokev", "ivars-talon-reward");
		const auto &tarlator = story_for("smokev", "tarlators-humanity-request");
		const auto &raltron = story_for("smokev", "raltrons-helm-delivery");
		const auto &ilvorntas = story_for("smokev", "ilvorntas-trophy-reward");
		const auto &meal = story_for("smokev", "a-meal-for-azcatlipoca");
		const auto &forvos = story_for("smokev", "forvos-bottle-exchange");
		service supplied_smoke(catalog);
		require(supplied_smoke.discover_zone(7, 42, 202, 20266, 100, "arrival") ==
				result::applied,
			"Smokeveil discovery failed");
		for (const auto &contact : smoke.contacts)
			require(supplied_smoke.meet_npc(7, 42, contact.mob_vnum, 20266, 101) ==
					result::applied,
				"Smokeveil fixture encounter failed");
		const auto before_smoke_read = supplied_smoke.serialize_state();
		supplies.carried.clear();
		supplies.carried[20252] = supplies.carried[20209] = supplies.carried[20210] = 1;
		journal = supplied_smoke.render_journal(7, 42, 202, 10, 1, 102, false, false,
							&supplies);
		require(journal.find("Next: " + raltron.steps.back().text) != std::string::npos &&
				journal.find("Next: " + raltron.steps.front().text) ==
					std::string::npos &&
				journal.find("Next: " + ivar_hearts.steps.front().text) !=
					std::string::npos &&
				journal.find("Next: " + meal.steps.front().text) !=
					std::string::npos,
			"supplied helm required producer history or wrong trophies satisfied a request");
		supplies.carried[20200] = supplies.carried[20211] = 1;
		journal = supplied_smoke.render_journal(7, 42, 202, 10, 1, 102, false, false,
							&supplies);
		require(journal.find("Next: " + ivar_hearts.steps.back().text) !=
					std::string::npos &&
				journal.find("Next: " + meal.steps.back().text) !=
					std::string::npos &&
				supplied_smoke.serialize_state() == before_smoke_read &&
				supplied_smoke.progress_for_zone(7, 42, 202).completed == 0,
			"exact Smokeveil proofs failed or reading/possession created history");
		record(supplied_smoke, raltron.contracts.front(), "supplied-helm", 202, 20272);
		record(supplied_smoke, forvos.contracts.front(), "bottle-supply", 202, 20279);
		require(supplied_smoke.progress_for_zone(7, 42, 202).completed == 1 &&
				supplied_smoke.progress_for_zone(7, 42, 202).total == 10,
			"Forvos service added an achievement or supplied Raltron helm needed Tarlator");
		record(supplied_smoke, ivar_talon.contracts.front(), "ivar-talon", 202, 20269);
		require(supplied_smoke.progress_for_zone(7, 42, 202).completed == 2,
			"Ivar talon receipt also completed the heart-pair request");

		service prepared_smoke(catalog);
		require(prepared_smoke.deserialize_state(before_smoke_read, &error),
			"Smokeveil encounter fixture reload failed");
		supplies.carried.clear();
		supplies.carried[20244] = supplies.carried[20253] = 1;
		journal = prepared_smoke.render_journal(7, 42, 202, 10, 1, 102, false, false,
							&supplies);
		require(journal.find("Next: " + tarlator.steps.back().text) != std::string::npos &&
				journal.find("Next: " + ilvorntas.steps.back().text) !=
					std::string::npos,
			"same-input requests were not independently ready before consumption");
		record(prepared_smoke, tarlator.contracts.front(), "tarlator-route", 202, 20267);
		supplies.carried.clear();
		journal = prepared_smoke.render_journal(7, 42, 202, 10, 1, 122, false, false,
							&supplies);
		require(prepared_smoke.progress_for_zone(7, 42, 202).completed == 1 &&
				journal.find("Next: " + raltron.steps[1].text) !=
					std::string::npos &&
				journal.find("Next: " + ilvorntas.steps.front().text) !=
					std::string::npos,
			"producer history recreated spent helm/trophies or completed the other recipient");
		for (const auto &request : smoke.stories)
			if (request.category != "service" && request.id != raltron.id &&
			    request.id != ivar_talon.id)
				record(supplied_smoke, request.contracts.front(),
				       request.id.c_str(), 202, 20266);
		service restored_smoke(catalog);
		require(restored_smoke.deserialize_state(supplied_smoke.serialize_state(),
							 &error) &&
				restored_smoke.progress_for_zone(7, 42, 202).completed == 10 &&
				restored_smoke.progress_for_zone(7, 42, 202).total == 10 &&
				restored_smoke.progress_for_zone(7, 42, 831).completed == 0,
			"Smokeveil reload changed receipts or invented Ravi's foreign completion");
		const auto &shards = story_for("caertannad", "four-distinct-life-shards");
		const auto &blackrock = story_for("caertannad", "marnys-blackrock-sample");
		const auto &endurium = story_for("caertannad", "marnys-endurium-commission");
		const auto &twin_staff = story_for("caertannad", "the-staff-of-twin-worlds");
		const auto &figurine = story_for("caertannad", "hindiss-figurine-exchange");
		const auto &head = story_for("caertannad", "hindiss-thel-samar-proof");
		const auto &remedy = story_for("caertannad", "mungirs-silverleaf-remedy");
		const auto &keeps =
			*std::find_if(catalog.story_mappings.begin(), catalog.story_mappings.end(),
				      [](const auto &m) { return m.source_area == "caertannad"; });
		service supplied_keeps(catalog);
		require(supplied_keeps.discover_zone(7, 42, 784, 78504, 100, "arrival") ==
				result::applied,
			"Twin Keeps discovery failed");
		for (const auto &contact : keeps.contacts)
			require(supplied_keeps.meet_npc(7, 42, contact.mob_vnum, 78504, 101) ==
					result::applied,
				"Twin Keeps fixture encounter failed");
		const auto before_keeps_read = supplied_keeps.serialize_state();
		supplies.carried.clear();
		supplies.carried[78499] = 4;
		journal = supplied_keeps.render_journal(7, 42, 784, 10, 1, 102, false, false,
							&supplies);
		require(journal.find("Next: " + shards.steps[2].text) != std::string::npos,
			"four copies of one same-named shard satisfied four distinct kinds");
		for (int item : { 78513, 78514, 78515, 78465, 78477, 78480, 78486, 78492, 78459 })
			supplies.carried[item] = 1;
		journal = supplied_keeps.render_journal(7, 42, 784, 10, 1, 102, false, false,
							&supplies);
		require(journal.find("Next: " + shards.steps.back().text) != std::string::npos &&
				journal.find("Next: " + twin_staff.steps.back().text) !=
					std::string::npos &&
				journal.find("Next: " + figurine.steps.back().text) !=
					std::string::npos &&
				supplied_keeps.serialize_state() == before_keeps_read &&
				supplied_keeps.progress_for_zone(7, 42, 784).completed == 0,
			"supplied components needed producer history or possession invented progress");
		record(supplied_keeps, blackrock.contracts.front(), "blackrock-sample", 784, 78646);
		supplies.carried.clear();
		supplies.carried[78463] = 1;
		journal = supplied_keeps.render_journal(7, 42, 784, 10, 1, 122, false, false,
							&supplies);
		require(journal.find("Next: " + endurium.steps[1].text) != std::string::npos,
			"producer history replaced Marny's missing physical receipt");
		supplies.carried[78424] = 1;
		journal = supplied_keeps.render_journal(7, 42, 784, 10, 1, 122, false, false,
							&supplies);
		require(journal.find("Next: " + endurium.steps.back().text) != std::string::npos &&
				journal.find("Turn-in currently unavailable:") != std::string::npos,
			"exact receipt did not ready the recipe or mixed-fee service lacked a warning");
		record(supplied_keeps, twin_staff.contracts.front(), "supplied-twin-staff", 784,
		       78823);
		record(supplied_keeps, figurine.contracts.front(), "supplied-figurine", 784, 78820);
		record(supplied_keeps, remedy.contracts.front(), "recovered-potion-service", 784,
		       78719);
		require(supplied_keeps.progress_for_zone(7, 42, 784).completed == 3 &&
				supplied_keeps.progress_for_zone(7, 42, 784).total == 29,
			"service added an achievement or a finale completed producer/head requests");
		record(supplied_keeps, head.contracts.front(), "independent-head", 784, 78820);
		for (const auto &request : keeps.stories)
			if (request.category != "service" && request.id != blackrock.id &&
			    request.id != twin_staff.id && request.id != figurine.id &&
			    request.id != head.id)
				record(supplied_keeps, request.contracts.front(),
				       request.id.c_str(), 784, 78504);
		service restored_keeps(catalog);
		require(restored_keeps.deserialize_state(supplied_keeps.serialize_state(),
							 &error) &&
				restored_keeps.progress_for_zone(7, 42, 784).completed == 29 &&
				restored_keeps.progress_for_zone(7, 42, 55).completed == 0 &&
				restored_keeps.progress_for_zone(7, 42, 831).completed == 0,
			"Twin Keeps reload changed independent receipts or invented foreign key/collector credit");
		const auto &quarters = story_for("bs", "the-four-bloodstone-quarters");
		const auto &wife = story_for("bs", "the-numbaca-ingredients");
		const auto &captain = story_for("bs", "the-captains-missionary-proof");
		const auto &bs_storm = story_for("bs", "navift-commission-55315");
		const auto &bs_cosmos = story_for("bs", "navift-commission-55318");
		const auto &bs_elixirs = story_for("bs", "fibblefingers-planar-elixirs");
		const auto &bs_earrings = story_for("bs", "hedvigs-nine-earring-service");
		const auto &bs_strength = story_for("bs", "hedvig-commission-55343");
		const auto &bloodstone =
			*std::find_if(catalog.story_mappings.begin(), catalog.story_mappings.end(),
				      [](const auto &m) { return m.source_area == "bs"; });
		service supplied_bs(catalog);
		require(supplied_bs.discover_zone(7, 42, 740, 74000, 100, "arrival") ==
				result::applied,
			"Bloodstone discovery failed");
		journal =
			supplied_bs.render_journal(7, 42, 740, 10, 1, 102, false, false, &supplies);
		require(journal.find("] " + quarters.title + "\r\n") == std::string::npos,
			"Bloodstone discovery exposed an unseen giver's story");
		for (const auto &contact : bloodstone.contacts)
			require(supplied_bs.meet_npc(7, 42, contact.mob_vnum, 74000, 101) ==
					result::applied,
				"Bloodstone fixture encounter failed");
		// Several recipes share a giver and final-step text. Inspect the owning row
		// so another ready recipe cannot conceal a missing ingredient in this one.
		const auto bs_section = [&](const auto &story)
		{
			const auto at = journal.find("] " + story.title + "\r\n");
			require(at != std::string::npos, "Bloodstone journal row missing");
			const auto end = journal.find("\r\n  [", at);
			return journal.substr(at, end == std::string::npos ? end : end - at);
		};
		const auto before_bs_read = supplied_bs.serialize_state();
		supplies.carried.clear();
		supplies.carried[74259] = 4;
		journal =
			supplied_bs.render_journal(7, 42, 740, 10, 1, 102, false, false, &supplies);
		require(bs_section(quarters).find("Next: " + quarters.steps[4].text) !=
				std::string::npos,
			"four copies of one quarter satisfied four different same-named kinds");
		for (int item : { 74260, 74261, 74262, 74057, 74240, 74243, 74246, 74257, 55166,
				  55209, 55284, 55316, 55317, 55167, 55319 })
			supplies.carried[item] = 1;
		journal =
			supplied_bs.render_journal(7, 42, 740, 10, 1, 102, false, false, &supplies);
		require(bs_section(quarters).find("Next: " + quarters.steps.back().text) !=
					std::string::npos &&
				bs_section(bs_storm).find("Next: " + bs_storm.steps.back().text) !=
					std::string::npos &&
				bs_section(wife).find("Next: " + wife.steps[5].text) !=
					std::string::npos &&
				bs_section(bs_cosmos).find("Next: " + bs_cosmos.steps[4].text) !=
					std::string::npos &&
				supplied_bs.serialize_state() == before_bs_read &&
				supplied_bs.progress_for_zone(7, 42, 740).completed == 0,
			"supplied finale needed history, original head/two-dust checks failed, or a read wrote progress");
		record(supplied_bs, captain.contracts.front(), "bs-captain", 740, 74587);
		journal =
			supplied_bs.render_journal(7, 42, 740, 10, 1, 122, false, false, &supplies);
		require(bs_section(wife).find("Next: " + wife.steps[5].text) != std::string::npos,
			"captain receipt replaced the physical transformed head");
		supplies.carried[74293] = 1;
		supplies.carried[55319] = 2;
		journal =
			supplied_bs.render_journal(7, 42, 740, 10, 1, 122, false, false, &supplies);
		require(bs_section(wife).find("Next: " + wife.steps.back().text) !=
					std::string::npos &&
				bs_section(bs_cosmos).find("Next: " +
							   bs_cosmos.steps.back().text) !=
					std::string::npos,
			"exact transformed head or second dust failed to ready its recipe");
		supplies.carried.clear();
		for (int item = 55136; item <= 55140; ++item)
			supplies.carried[item] = 1;
		journal =
			supplied_bs.render_journal(7, 42, 740, 10, 1, 122, false, false, &supplies);
		require(bs_section(bs_elixirs).find("Next: " + bs_elixirs.steps[10].text) !=
				std::string::npos,
			"five potions alone satisfied Bloodstone's eleven-item elixir recipe");
		for (int item = 55198; item <= 55203; ++item)
			supplies.carried[item] = 1;
		for (int item = 55343; item <= 55351; ++item)
			supplies.carried[item] = 1;
		supplies.carried[55130] = supplies.carried[55352] = supplies.carried[55353] = 1;
		journal =
			supplied_bs.render_journal(7, 42, 740, 10, 1, 122, false, false, &supplies);
		require(bs_section(bs_elixirs).find("Next: " + bs_elixirs.steps.back().text) !=
					std::string::npos &&
				bs_section(bs_earrings).find("Turn-in currently unavailable:") !=
					std::string::npos &&
				bs_section(bs_strength).find("Next: " + bs_strength.steps[1].text) !=
					std::string::npos,
			"eleven-item recipe, mixed-fee warning, or matching-scroll requirement failed");
		supplies.carried[55352] = 2;
		journal =
			supplied_bs.render_journal(7, 42, 740, 10, 1, 122, false, false, &supplies);
		require(bs_section(bs_strength).find("Next: " + bs_strength.steps.back().text) !=
				std::string::npos,
			"two matching scrolls failed to ready the earring service");
		record(supplied_bs, storm.contracts.front(), "bs-foreign-storm", 550, 55125);
		require(supplied_bs.progress_for_zone(7, 42, 740).completed == 1,
			"Winterhaven's equal-output storm recipe completed Bloodstone's recipe");
		for (const auto &request : bloodstone.stories)
			if (request.category == "service")
				record(supplied_bs, request.contracts.front(), request.id.c_str(),
				       740, 74000);
		for (const auto &[id, reason] : bloodstone.exclusions)
			record(supplied_bs, id, id.c_str(), 740, 74000);
		require(supplied_bs.progress_for_zone(7, 42, 740).completed == 1 &&
				supplied_bs.progress_for_zone(7, 42, 740).total == 31,
			"Bloodstone services or rejection/invalid receipts added achievements");
		record(supplied_bs, quarters.contracts.front(), "bs-supplied-quarters", 740, 74324);
		record(supplied_bs, bs_storm.contracts.front(), "bs-supplied-storm", 740, 74000);
		require(supplied_bs.progress_for_zone(7, 42, 740).completed == 3,
			"final quarter/artifact receipts completed earlier producer commissions");
		for (const auto &request : bloodstone.stories)
			if (request.category != "service" && request.id != captain.id &&
			    request.id != quarters.id && request.id != bs_storm.id)
				record(supplied_bs, request.contracts.front(), request.id.c_str(),
				       740, 74000);
		service restored_bs(catalog);
		require(restored_bs.deserialize_state(supplied_bs.serialize_state(), &error) &&
				restored_bs.progress_for_zone(7, 42, 740).completed == 31 &&
				restored_bs.progress_for_zone(7, 42, 550).completed == 1,
			"Bloodstone recovery changed independent receipts or invented foreign producers");
		const auto &runes = story_for("moria", "malchors-five-runes");
		service supplied_runes(catalog);
		require(supplied_runes.discover_zone(7, 42, 990, 99001, 100, "arrival") ==
				result::applied,
			"Neverwinter discovery failed");
		supplies.carried.clear();
		journal = supplied_runes.render_journal(7, 42, 990, 10, 1, 102, false, false,
							&supplies);
		require(journal.find("] " + runes.title + "\r\n") == std::string::npos,
			"Neverwinter discovery exposed an unmet Malchor's request");
		require(supplied_runes.meet_npc(7, 42, 99028, 99266, 101) == result::applied,
			"Malchor encounter failed");
		const auto before_rune_read = supplied_runes.serialize_state();
		supplies.carried[99002] = 5;
		journal = supplied_runes.render_journal(7, 42, 990, 10, 1, 102, false, false,
							&supplies);
		require(journal.find("Next: " + runes.steps[2].text) != std::string::npos,
			"five amethyst runes replaced the five distinct rune kinds");
		for (int item = 99003; item <= 99006; ++item)
			supplies.carried[item] = 1;
		journal = supplied_runes.render_journal(7, 42, 990, 10, 1, 102, false, false,
							&supplies);
		require(journal.find("Next: " + runes.steps.back().text) != std::string::npos &&
				journal.find("Next: " + runes.steps.front().text) ==
					std::string::npos &&
				supplied_runes.serialize_state() == before_rune_read &&
				supplied_runes.progress_for_zone(7, 42, 990).completed == 0,
			"supplied runes needed bridge history or a readiness read awarded credit");
		supplies.carried.erase(99004);
		journal = supplied_runes.render_journal(7, 42, 990, 10, 1, 103, false, false,
							&supplies);
		require(journal.find("Next: " + runes.steps[3].text) != std::string::npos,
			"a spent diamond rune remained ready");
		for (const auto &id : runes.contracts)
			record(supplied_runes, id, id.c_str(), 990, 99266);
		require(supplied_runes.progress_for_zone(7, 42, 990).completed == 1 &&
				supplied_runes.progress_for_zone(7, 42, 990).total == 1,
			"historical equal-offering reward variants added multiple achievements");
		service restored_runes(catalog);
		require(restored_runes.deserialize_state(supplied_runes.serialize_state(),
							 &error) &&
				restored_runes.progress_for_zone(7, 42, 990).completed == 1 &&
				restored_runes.progress_for_zone(7, 42, 990).total == 1,
			"Neverwinter reward-family recovery changed progress");
		const auto &claw_final = story_for("clwcvrn", "the-rainbow-key-and-the-king");
		const auto &claw_blue = story_for("clwcvrn", "blue-shield");
		const auto &claw_violet = story_for("clwcvrn", "violet-collar");
		const auto &claw_sage = story_for("clwcvrn", "the-sages-paid-secret");
		const auto &claw =
			*std::find_if(catalog.story_mappings.begin(), catalog.story_mappings.end(),
				      [](const auto &m) { return m.source_area == "clwcvrn"; });
		service supplied_claw(catalog);
		require(supplied_claw.discover_zone(7, 42, 807, 80700, 100, "arrival") ==
				result::applied,
			"Clawed Caverns discovery failed");
		supplies.carried.clear();
		journal = supplied_claw.render_journal(7, 42, 807, 10, 1, 102, false, false,
						       &supplies);
		require(journal.find("] " + claw_final.title + "\r\n") == std::string::npos,
			"Clawed Caverns discovery exposed an unseen king's story");
		for (const auto &contact : claw.contacts)
			require(supplied_claw.meet_npc(7, 42, contact.mob_vnum, 80700, 101) ==
					result::applied,
				"Clawed Caverns fixture encounter failed");
		const auto claw_section = [&](const auto &story)
		{
			const auto at = journal.find("] " + story.title + "\r\n");
			require(at != std::string::npos, "Clawed Caverns journal row missing");
			const auto end = journal.find("\r\n  [", at);
			return journal.substr(at, end == std::string::npos ? end : end - at);
		};
		const auto before_claw_read = supplied_claw.serialize_state();
		supplies.carried[80733] = supplies.carried[80730] = supplies.carried[80708] = 1;
		journal = supplied_claw.render_journal(7, 42, 807, 10, 1, 102, false, false,
						       &supplies);
		require(claw_section(claw_final).find("Next: " + claw_final.steps[2].text) !=
					std::string::npos &&
				claw_section(claw_blue).find("Next: " +
							     claw_blue.steps.back().text) !=
					std::string::npos &&
				claw_section(claw_violet)
						.find("Next: " + claw_violet.steps[0].text) !=
					std::string::npos &&
				claw_section(claw_sage).find(
					"currently unavailable under active accounting") !=
					std::string::npos,
			"intact key/pile replaced rainbow shards, wrong color readied shaping, or fee warning was absent");
		supplies.carried.clear();
		supplies.carried[80734] = 1;
		journal = supplied_claw.render_journal(7, 42, 807, 10, 1, 103, false, false,
						       &supplies);
		require(claw_section(claw_final).find("Next: " + claw_final.steps.back().text) !=
					std::string::npos &&
				supplied_claw.serialize_state() == before_claw_read,
			"supplied rainbow shards needed optional keys/history or readiness mutated progress");
		for (const auto &request : claw.stories)
			if (request.category == "service")
				record(supplied_claw, request.contracts.front(), request.id.c_str(),
				       807, 80700);
		for (const auto &[id, reason] : claw.exclusions)
			record(supplied_claw, id, id.c_str(), 807, 80700);
		require(supplied_claw.progress_for_zone(7, 42, 807).completed == 0 &&
				supplied_claw.progress_for_zone(7, 42, 807).total == 1,
			"Clawed Caverns shaping, paid clue or returned offers awarded achievements");
		record(supplied_claw, claw_final.contracts.front(), "claw-supplied-final", 807,
		       80750);
		service restored_claw(catalog);
		require(restored_claw.deserialize_state(supplied_claw.serialize_state(), &error) &&
				restored_claw.progress_for_zone(7, 42, 807).completed == 1 &&
				restored_claw.progress_for_zone(7, 42, 807).total == 1,
			"Clawed Caverns recovery changed the independent final delivery");
		const auto &long_mayor = story_for("long", "proof-against-the-siege-leaders");
		const auto &long_solar = story_for("long", "recognition-by-selunes-solar");
		const auto &long_mist = story_for("long", "the-shadowy-mist");
		const auto &long_blend = story_for("long", "the-kiss-of-talona-blend");
		const auto &long_viper = story_for("long", "vipers-delight");
		const auto &long_boots = story_for("long", "four-skins-for-snakeskin-boots");
		const auto &long_fish = story_for("long", "the-fishscale-potion-experiment");
		const auto &long_map =
			*std::find_if(catalog.story_mappings.begin(), catalog.story_mappings.end(),
				      [](const auto &m) { return m.source_area == "long"; });
		service supplied_long(catalog);
		require(supplied_long.discover_zone(7, 42, 344, 34401, 100, "arrival") ==
				result::applied,
			"Longhollow discovery failed");
		supplies.carried.clear();
		journal = supplied_long.render_journal(7, 42, 344, 10, 1, 102, false, false,
						       &supplies);
		require(journal.find("] " + long_mayor.title + "\r\n") == std::string::npos,
			"Longhollow discovery exposed an unseen mayor's story");
		for (const auto &contact : long_map.contacts)
			require(supplied_long.meet_npc(7, 42, contact.mob_vnum, 34401, 101) ==
					result::applied,
				"Longhollow fixture encounter failed");
		const auto long_section = [&](const auto &story)
		{
			const auto at = journal.find("] " + story.title + "\r\n");
			require(at != std::string::npos, "Longhollow journal row missing");
			const auto end = journal.find("\r\n  [", at);
			return journal.substr(at, end == std::string::npos ? end : end - at);
		};
		for (const auto &request : long_map.stories)
			if (request.category == "service")
				record(supplied_long, request.contracts.front(), request.id.c_str(),
				       344, 34411);
		for (const auto &[id, reason] : long_map.exclusions)
			record(supplied_long, id, id.c_str(), 344, 34417);
		require(supplied_long.progress_for_zone(7, 42, 344).completed == 0 &&
				supplied_long.progress_for_zone(7, 42, 344).total == 9,
			"clothing or empty Rolane placeholder awarded a Longhollow achievement");
		const auto before_long_read = supplied_long.serialize_state();
		supplies.carried[34427] = 5;
		supplies.carried[34464] = supplies.carried[34406] = supplies.carried[34433] =
			supplies.carried[34413] = 1;
		journal = supplied_long.render_journal(7, 42, 344, 10, 1, 102, false, false,
						       &supplies);
		require(long_section(long_mayor).find("Next: " + long_mayor.steps[1].text) !=
					std::string::npos &&
				long_section(long_solar).find("Next: " + long_solar.steps[1].text) !=
					std::string::npos &&
				long_section(long_viper).find("Next: " + long_viper.steps[0].text) !=
					std::string::npos &&
				long_section(long_boots).find("(1/4)") != std::string::npos,
			"duplicate heads, moonstone, one sac or one skin replaced exact ingredients");
		supplies.carried[34413] = 4;
		supplies.carried[34406] = 2;
		journal = supplied_long.render_journal(7, 42, 344, 10, 1, 103, false, false,
						       &supplies);
		require(long_section(long_viper).find("Next: " + long_viper.steps.back().text) !=
					std::string::npos &&
				long_section(long_boots).find("Turn-in currently unavailable:") !=
					std::string::npos &&
				long_section(long_boots).find("20 gold") != std::string::npos &&
				long_section(long_fish).find("25,000 copper") !=
					std::string::npos &&
				long_section(long_fish).find("Turn-in currently unavailable:") ==
					std::string::npos,
			"duplicate counts or coin payment versus native cash reward were conflated");
		supplies.carried.clear();
		for (int item : { 34452, 34443, 34444, 34445, 34439, 34449, 34447 })
			supplies.carried[item] = 1;
		supplies.carried[34425] = 1;
		journal = supplied_long.render_journal(7, 42, 344, 10, 1, 104, false, false,
						       &supplies);
		require(long_section(long_solar).find("Next: " + long_solar.steps.back().text) !=
					std::string::npos &&
				long_section(long_mist).find("Next: " +
							     long_mist.steps.back().text) !=
					std::string::npos &&
				long_section(long_blend).find("Next: " + long_blend.steps[4].text) !=
					std::string::npos &&
				supplied_long.serialize_state() == before_long_read,
			"supplied finals needed producer history, rooted vine replaced bloom, or reads mutated state");
		record(supplied_long, long_solar.contracts.front(), "long-supplied-bracer", 344,
		       34417);
		require(supplied_long.progress_for_zone(7, 42, 344).completed == 1,
			"Solar's supplied-bracer receipt invented the mayor's earlier completion");
		record(supplied_long, long_mayor.contracts.front(), "long-mayor", 344, 34446);
		for (const auto *finale : { &long_mist, &long_blend })
			for (const auto &step : finale->steps)
				if (step.optional)
					record(supplied_long, step.contracts.front(),
					       step.contracts.front().c_str(), 344, 34438);
		supplies.carried.clear();
		journal = supplied_long.render_journal(7, 42, 344, 10, 1, 105, false, false,
						       &supplies);
		require(long_section(long_mist).find("Next: " + long_mist.steps[3].text) !=
					std::string::npos &&
				long_section(long_blend).find("Next: " + long_blend.steps[2].text) !=
					std::string::npos &&
				supplied_long.progress_for_zone(7, 42, 344).completed == 7,
			"producer receipts replaced spent physical outputs or completed later finales");
		record(supplied_long, long_mist.contracts.front(), "long-mist", 344, 34438);
		record(supplied_long, long_blend.contracts.front(), "long-blend", 344, 34438);
		service restored_long(catalog);
		require(restored_long.deserialize_state(supplied_long.serialize_state(), &error) &&
				restored_long.progress_for_zone(7, 42, 344).completed == 9 &&
				restored_long.progress_for_zone(7, 42, 344).total == 9,
			"Longhollow recovery changed independent story/request credit");
		const auto &pearl_rebuild =
			story_for("blackpearl", "reconstruct-warthehrs-dragonslayer");
		const auto &pearl_skin =
			story_for("blackpearl", "ghalasaxs-skin-and-warthehrs-reward");
		const auto &pearl_horns =
			story_for("blackpearl", "four-distinct-horns-for-derimous");
		const auto &pearl_reply = story_for("blackpearl", "abals-reply-for-lyles-fragment");
		const auto &pearl_cash =
			story_for("blackpearl", "the-gartham-report-and-travel-funds");
		const auto &pearl_gadget = story_for("blackpearl", "the-patrons-gadget-purchase");
		const auto &pearl_map =
			*std::find_if(catalog.story_mappings.begin(), catalog.story_mappings.end(),
				      [](const auto &m) { return m.source_area == "blackpearl"; });
		service supplied_pearl(catalog);
		supplies.carried.clear();
		require(supplied_pearl.discover_zone(7, 42, 135, 13556, 100, "arrival") ==
					result::applied &&
				supplied_pearl.meet_npc(7, 42, 142215, 13556, 101) ==
					result::applied &&
				!supplied_pearl.has_discovered(7, 42, 1422),
			"foreign campaign contact falsely discovered the owned wreck");
		require(supplied_pearl.discover_zone(7, 42, 1422, 142201, 102, "arrival") ==
				result::applied,
			"Black Pearl synthetic discovery failed");
		journal = supplied_pearl.render_journal(7, 42, 1422, 10, 1, 103, false, false,
							&supplies);
		require(journal.find("] " + pearl_skin.title + "\r\n") != std::string::npos &&
				journal.find("] " + pearl_rebuild.title + "\r\n") ==
					std::string::npos,
			"persisted foreign encounter was lost or exposed an unseen steelsmith");
		// Synthetic encounters qualify projection only; ordinary placements remain blocked.
		for (const auto &contact : pearl_map.contacts)
		{
			const auto met =
				supplied_pearl.meet_npc(7, 42, contact.mob_vnum, 142201, 104);
			require(met == result::applied || met == result::already_applied,
				"Black Pearl synthetic encounter failed");
		}
		const auto pearl_section = [&](const auto &story)
		{
			const auto at = journal.find("] " + story.title + "\r\n");
			require(at != std::string::npos, "Black Pearl journal row missing");
			const auto next = journal.find("\r\n  [", at);
			return journal.substr(at, next == std::string::npos ? next : next - at);
		};
		for (const auto &trade : pearl_map.stories)
			if (trade.category == "service")
				record(supplied_pearl, trade.contracts.front(), trade.id.c_str(),
				       1422, 142200);
		require(supplied_pearl.progress_for_zone(7, 42, 1422).completed == 0 &&
				supplied_pearl.progress_for_zone(7, 42, 1422).total == 14,
			"briefings, returned fragments or hunter trades added quest achievements");
		const auto before_pearl_read = supplied_pearl.serialize_state();
		supplies.carried[142204] = 8;
		for (const auto item : { 142207, 142210, 142211 })
			supplies.carried[item] = 1;
		supplies.carried[142220] = 4;
		supplies.carried[142213] = 2;
		journal = supplied_pearl.render_journal(7, 42, 1422, 10, 1, 121, false, false,
							&supplies);
		require(pearl_section(pearl_rebuild).find("Next: " + pearl_rebuild.steps[8].text) !=
					std::string::npos &&
				pearl_section(pearl_horns)
						.find("Next: " + pearl_horns.steps[1].text) !=
					std::string::npos &&
				pearl_section(pearl_reply)
						.find("Next: " + pearl_reply.steps[1].text) !=
					std::string::npos,
			"same-named fragment, horn or letter copies replaced distinct required kinds");
		supplies.carried.clear();
		for (const auto item : { 142204, 142205, 142206, 142208, 142209, 142224 })
			supplies.carried[item] = 1;
		for (const auto item : { 142207, 142210, 142211 })
			supplies.carried[item] = 1;
		journal = supplied_pearl.render_journal(7, 42, 1422, 10, 1, 122, false, false,
							&supplies);
		require(pearl_section(pearl_rebuild).find("Next: " + pearl_rebuild.steps[13].text) !=
				std::string::npos,
			"unexchanged original fragments satisfied the replacement-piece recipe");
		for (const auto item :
		     { 142226, 142227, 142228, 142218, 142219, 142220, 142221, 142222, 142223 })
			supplies.carried[item] = 1;
		journal = supplied_pearl.render_journal(7, 42, 1422, 10, 1, 123, false, false,
							&supplies);
		require(pearl_section(pearl_rebuild)
						.find("Next: " + pearl_rebuild.steps.back().text) !=
					std::string::npos &&
				pearl_section(pearl_skin)
						.find("Next: " + pearl_skin.steps.back().text) !=
					std::string::npos &&
				pearl_section(pearl_horns)
						.find("Next: " + pearl_horns.steps.back().text) !=
					std::string::npos &&
				pearl_section(pearl_reply)
						.find("Next: " + pearl_reply.steps.back().text) !=
					std::string::npos &&
				supplied_pearl.serialize_state() == before_pearl_read,
			"supplied exact pieces/proofs required personal preparation or mutated history");
		require(pearl_section(pearl_cash).find("100,000 copper") != std::string::npos &&
				pearl_section(pearl_cash).find("Turn-in currently unavailable:") ==
					std::string::npos &&
				pearl_section(pearl_gadget)
						.find("unavailable under active accounting") !=
					std::string::npos &&
				journal.find("exitless holding room") != std::string::npos,
			"cash reward, unsupported purchase or campaign source blockers were misrepresented");
		record(supplied_pearl, pearl_skin.contracts.front(), "pearl-supplied-skin", 1422,
		       142200);
		require(supplied_pearl.progress_for_zone(7, 42, 1422).completed == 1,
			"supplied skin required reconstruction or completed the whole campaign");
		record(supplied_pearl, pearl_rebuild.contracts.front(), "pearl-supplied-pieces",
		       1422, 142200);
		service restored_pearl(catalog);
		require(restored_pearl.deserialize_state(supplied_pearl.serialize_state(),
							 &error) &&
				restored_pearl.progress_for_zone(7, 42, 1422).completed == 2 &&
				restored_pearl.progress_for_zone(7, 42, 1422).total == 14,
			"Black Pearl recovery changed independent finales or credited services");
		service prepared_pearl(catalog);
		require(prepared_pearl.discover_zone(7, 42, 1422, 142201, 100, "arrival") ==
					result::applied &&
				prepared_pearl.meet_npc(7, 42, 142216, 142201, 101) ==
					result::applied,
			"Black Pearl preparation fixture failed");
		for (size_t i = 0; i < 7; ++i)
			record(prepared_pearl, pearl_rebuild.steps[i].contracts.front(),
			       pearl_rebuild.steps[i].id.c_str(), 1422, 142200);
		supplies.carried.clear();
		journal = prepared_pearl.render_journal(7, 42, 1422, 10, 1, 124, false, false,
							&supplies);
		require(pearl_section(pearl_rebuild).find("Next: " + pearl_rebuild.steps[7].text) !=
					std::string::npos &&
				prepared_pearl.progress_for_zone(7, 42, 1422).completed == 7,
			"producer receipts replaced spent physical pieces or completed reconstruction");
		const auto &chaplain = story_for("ravenloft2", "the-chaplains-favor");
		const auto &blinsky = story_for("ravenloft2", "blinskys-clockwork-recovery");
		const auto &rictavio = story_for("ravenloft2", "rictavios-strahd-finale");
		const auto &ezmerelda = story_for("ravenloft2", "ezmereldas-strahd-finale");
		const auto &jander = story_for("ravenloft2", "janders-strahd-finale");
		const auto &lich = story_for("ravenloft2", "exethanters-lost-memory");
		const auto &witch = story_for("ravenloft2", "thredras-rejuvenation");
		const auto &wine = story_for("ravenloft2", "izeks-elven-wine");
		const auto &reading = story_for("ravenloft2", "ezmereldas-paid-reading");
		const auto &raven_map =
			*std::find_if(catalog.story_mappings.begin(), catalog.story_mappings.end(),
				      [](const auto &m) { return m.source_area == "ravenloft2"; });
		service supplied_raven(catalog);
		require(supplied_raven.discover_zone(7, 42, 910, 91121, 100, "arrival") ==
					result::applied &&
				supplied_raven.meet_npc(7, 42, 59081, 91121, 101) ==
					result::applied &&
				!supplied_raven.has_discovered(7, 42, 590),
			"foreign toyshop encounter falsely discovered the catacombs");
		require(supplied_raven.discover_zone(7, 42, 590, 59057, 102, "arrival") ==
				result::applied,
			"Ravenloft synthetic discovery failed");
		for (const auto &contact : raven_map.contacts)
		{
			const auto met =
				supplied_raven.meet_npc(7, 42, contact.mob_vnum, 59057, 103);
			require(met == result::applied || met == result::already_applied,
				"Ravenloft synthetic encounter failed");
		}
		const auto raven_section = [&](const auto &story)
		{
			const auto at = journal.find("] " + story.title + "\r\n");
			require(at != std::string::npos, "Ravenloft journal row missing");
			const auto next = journal.find("\r\n  [", at);
			return journal.substr(at, next == std::string::npos ? next : next - at);
		};
		// Synthetic receipts test projection; unsupported paid journeys remain unqualified.
		for (const auto &trade : raven_map.stories)
			if (trade.category == "service")
				record(supplied_raven, trade.contracts.front(), trade.id.c_str(),
				       590, 59057);
		require(supplied_raven.progress_for_zone(7, 42, 590).completed == 0 &&
				supplied_raven.progress_for_zone(7, 42, 590).total == 25,
			"fortune, key, food or paid services added quest achievements");
		supplies.carried.clear();
		supplies.carried[59202] = 4;
		supplies.carried[59300] = 1;
		journal = supplied_raven.render_journal(7, 42, 590, 10, 1, 121, false, false,
							&supplies);
		require(raven_section(chaplain).find("Next: " + chaplain.steps[1].text) !=
				std::string::npos,
			"four physical coins satisfied the five-coin commission");
		supplies.carried[59202] = 5;
		supplies.equipped[16] = 59281;
		journal = supplied_raven.render_journal(7, 42, 590, 10, 1, 122, false, false,
							&supplies);
		require(raven_section(chaplain).find("Next: " + chaplain.steps[2].text) !=
				std::string::npos,
			"wrong-role or equipped scroll satisfied the carried Chaplain scroll");
		supplies.carried[59281] = 1;
		supplies.carried[59269] = 1;
		supplies.carried[59255] = 1;
		supplies.carried[59150] = 1;
		const auto before_raven_read = supplied_raven.serialize_state();
		journal = supplied_raven.render_journal(7, 42, 590, 10, 1, 123, false, false,
							&supplies);
		for (const auto *story :
		     { &chaplain, &blinsky, &rictavio, &ezmerelda, &jander, &lich, &witch })
			require(raven_section(*story).find("Next: " + story->steps.back().text) !=
					std::string::npos,
				"supplied Ravenloft materials required personal history or all alternatives");
		require(supplied_raven.serialize_state() == before_raven_read,
			"Ravenloft inventory readiness awarded history");
		require(raven_section(wine).find("250,000 copper") != std::string::npos &&
				raven_section(wine).find("Turn-in currently unavailable:") ==
					std::string::npos &&
				raven_section(reading).find(
					"unavailable under active accounting") != std::string::npos,
			"supported cash reward and unsupported wallet offering were confused");
		record(supplied_raven, blinsky.contracts.front(), "raven-blinsky-one", 590, 59355);
		record(supplied_raven, blinsky.contracts.back(), "raven-blinsky-two", 590, 59355);
		require(supplied_raven.progress_for_zone(7, 42, 590).completed == 1,
			"two clockwork alternatives awarded two recovery achievements");
		record(supplied_raven, rictavio.contracts.front(), "raven-rictavio", 590, 59356);
		journal = supplied_raven.render_journal(7, 42, 590, 10, 1, 124, false, false,
							&supplies);
		require(supplied_raven.progress_for_zone(7, 42, 590).completed == 2 &&
				raven_section(ezmerelda).find("Next: " +
							      ezmerelda.steps.back().text) !=
					std::string::npos &&
				raven_section(jander).find("Next: " + jander.steps.back().text) !=
					std::string::npos,
			"one skull recipient completed the other distinct finales");
		record(supplied_raven, lich.contracts.front(), "raven-lich", 590, 59334);
		record(supplied_raven, witch.contracts.front(), "raven-witch", 590, 59050);
		record(supplied_raven, chaplain.contracts.front(), "raven-chaplain", 590, 59000);
		service restored_raven(catalog);
		require(restored_raven.deserialize_state(supplied_raven.serialize_state(),
							 &error) &&
				restored_raven.progress_for_zone(7, 42, 590).completed == 5 &&
				restored_raven.progress_for_zone(7, 42, 590).total == 25,
			"Ravenloft recovery changed alternative, recipient or service credit");
		service prepared_raven(catalog);
		require(prepared_raven.discover_zone(7, 42, 590, 59057, 100, "arrival") ==
					result::applied &&
				prepared_raven.meet_npc(7, 42, 59070, 59057, 101) ==
					result::applied,
			"Ravenloft optional proof fixture failed");
		record(prepared_raven, chaplain.steps[0].contracts.front(), "raven-one-proof", 590,
		       59000);
		supplies.carried.clear();
		supplies.carried[59281] = 1;
		journal = prepared_raven.render_journal(7, 42, 590, 10, 1, 125, false, false,
							&supplies);
		require(raven_section(chaplain).find("Next: " + chaplain.steps[1].text) !=
					std::string::npos &&
				prepared_raven.progress_for_zone(7, 42, 590).completed == 1,
			"one historical proof replaced five current coins or completed a favor");
		const auto &barovia_collection = story_for("barovia", "bildraths-nine-trinkets");
		const auto &barovia_brooch = story_for("barovia", "ashlyns-lost-companions");
		const auto &barovia_letter = story_for("barovia", "kolyans-forged-letter");
		const auto &barovia_plan = story_for("barovia", "hossas-ambush-plan");
		const auto &barovia_heart = story_for("barovia", "ephons-gate-key");
		const auto &barovia_daughter = story_for("barovia", "gertruda-and-mad-mary");
		const auto &barovia_clue = story_for("barovia", "parriwimples-collection-clue");
		const auto &barovia_map =
			*std::find_if(catalog.story_mappings.begin(), catalog.story_mappings.end(),
				      [](const auto &m) { return m.source_area == "barovia"; });
		service supplied_barovia(catalog);
		require(supplied_barovia.discover_zone(7, 42, 910, 91000, 100, "arrival") ==
				result::applied,
			"Barovia discovery fixture failed");
		journal = supplied_barovia.render_journal(7, 42, 910, 10, 1, 101, false, false);
		require(journal.find("] " + barovia_collection.title) == std::string::npos,
			"unseen Bildrath exposed his journal row");
		for (const auto &contact : barovia_map.contacts)
			require(supplied_barovia.meet_npc(7, 42, contact.mob_vnum, 91000, 102) ==
					result::applied,
				"Barovia encounter fixture failed");
		const auto barovia_section = [&](const auto &story)
		{
			const auto at = journal.find("] " + story.title + "\r\n");
			require(at != std::string::npos, "Barovia journal row missing");
			const auto next = journal.find("\r\n  [", at);
			return journal.substr(at, next == std::string::npos ? next : next - at);
		};
		// Synthetic service receipts qualify projection, not unavailable paid gameplay.
		for (const auto &trade : barovia_map.stories)
			if (trade.category == "service")
				record(supplied_barovia, trade.contracts.front(), trade.id.c_str(),
				       910, 91000);
		require(supplied_barovia.progress_for_zone(7, 42, 910).completed == 0 &&
				supplied_barovia.progress_for_zone(7, 42, 910).total == 6,
			"Barovia guidance services added achievements");
		supplies.carried.clear();
		supplies.equipped.clear();
		supplies.carried[91042] = 9;
		journal = supplied_barovia.render_journal(7, 42, 910, 10, 1, 121, false, false,
							  &supplies);
		require(barovia_section(barovia_collection)
					.find("Next: " + barovia_collection.steps[2].text) !=
				std::string::npos,
			"nine identical trinkets replaced the exact nine-kind collection");
		supplies.carried.clear();
		for (const auto item : { 91015, 91018, 91026, 91027, 91042, 91043, 91044, 91046 })
			supplies.carried[item] = 1;
		supplies.equipped[24] = 91045;
		journal = supplied_barovia.render_journal(7, 42, 910, 10, 1, 122, false, false,
							  &supplies);
		require(barovia_section(barovia_collection)
					.find("Next: " + barovia_collection.steps[2].text) !=
				std::string::npos,
			"equipped hair pin satisfied the directly carried offering");
		supplies.carried[91045] = 1;
		supplies.carried.erase(91027);
		supplies.equipped[21] = 91027;
		journal = supplied_barovia.render_journal(7, 42, 910, 10, 1, 123, false, false,
							  &supplies);
		require(barovia_section(barovia_collection)
					.find("Next: " + barovia_collection.steps[9].text) !=
				std::string::npos,
			"equipped ring replaced the carried collection kind");
		supplies.carried[91027] = 1;
		supplies.carried.erase(91026);
		journal = supplied_barovia.render_journal(7, 42, 910, 10, 1, 124, false, false,
							  &supplies);
		require(barovia_section(barovia_collection)
					.find("Next: " + barovia_collection.steps[8].text) !=
				std::string::npos,
			"missing physical electrum coin was satisfied by other trinkets or paid history");
		for (const auto item : { 91026, 91021, 91022, 91038, 91036, 58412 })
			supplies.carried[item] = 1;
		const auto before_barovia_read = supplied_barovia.serialize_state();
		journal = supplied_barovia.render_journal(7, 42, 910, 10, 1, 125, false, false,
							  &supplies);
		for (const auto *story : { &barovia_collection, &barovia_brooch, &barovia_letter,
					   &barovia_plan, &barovia_heart, &barovia_daughter })
			require(barovia_section(*story).find("Next: " + story->steps.back().text) !=
					std::string::npos,
				"supplied Barovia final required optional source, key, note, kill or escort history");
		require(supplied_barovia.serialize_state() == before_barovia_read,
			"Barovia readiness awarded personal source or campaign history");
		require(barovia_section(barovia_letter).find("200,000 copper") !=
					std::string::npos &&
				barovia_section(barovia_letter)
						.find("Turn-in currently unavailable:") ==
					std::string::npos &&
				barovia_section(barovia_clue)
						.find("unavailable under active accounting") !=
					std::string::npos,
			"supported letter cash reward was confused with unsupported clue fee");
		record(supplied_barovia, barovia_plan.contracts.front(), "barovia-supplied-plan",
		       910, 91119);
		require(supplied_barovia.progress_for_zone(7, 42, 910).completed == 1,
			"ambush-plan delivery required or completed the first note");
		for (const auto *story : { &barovia_brooch, &barovia_heart, &barovia_daughter,
					   &barovia_collection, &barovia_letter })
			record(supplied_barovia, story->contracts.front(), story->id.c_str(), 910,
			       91000);
		service restored_barovia(catalog);
		require(restored_barovia.deserialize_state(supplied_barovia.serialize_state(),
							   &error) &&
				restored_barovia.progress_for_zone(7, 42, 910).completed == 6 &&
				restored_barovia.progress_for_zone(7, 42, 910).total == 6,
			"Barovia receipt recovery changed independent finals or counted services");
		service prepared_barovia(catalog);
		require(prepared_barovia.discover_zone(7, 42, 910, 91000, 100, "arrival") ==
					result::applied &&
				prepared_barovia.meet_npc(7, 42, 91011, 91119, 101) ==
					result::applied,
			"Barovia preparation fixture failed");
		record(prepared_barovia, barovia_letter.steps[0].contracts.front(),
		       "barovia-ireena-note", 910, 91134);
		supplies.carried.clear();
		supplies.equipped.clear();
		journal = prepared_barovia.render_journal(7, 42, 910, 10, 1, 126, false, false,
							  &supplies);
		require(barovia_section(barovia_letter)
						.find("Next: " + barovia_letter.steps[1].text) !=
					std::string::npos &&
				prepared_barovia.progress_for_zone(7, 42, 910).completed == 0,
			"Ireena history replaced a spent first note or awarded Ismark completion");
		record(prepared_barovia, barovia_letter.contracts.front(), "barovia-first-letter",
		       910, 91119);
		journal = prepared_barovia.render_journal(7, 42, 910, 10, 1, 127, false, false,
							  &supplies);
		require(barovia_section(barovia_plan).find("Next: " + barovia_plan.steps[2].text) !=
					std::string::npos &&
				prepared_barovia.progress_for_zone(7, 42, 910).completed == 1,
			"letter briefing replaced the absent ambush plan or completed its independent quest");
		const auto &tikitt_map =
			*std::find_if(catalog.story_mappings.begin(), catalog.story_mappings.end(),
				      [](const auto &m) { return m.source_area == "tikitt"; });
		const auto &treasure_key = story_for("tikitt", "assemble-the-royal-treasure-key");
		const auto &mirror = story_for("tikitt", "a-mirror-for-the-drow");
		const auto &clover = story_for("tikitt", "ogallaghers-lost-luck");
		const auto &zynar = story_for("tikitt", "armor-for-zynar");
		const auto &mangler = story_for("tikitt", "forge-the-madmans-mangler");
		service supplied_tikitt(catalog);
		require(supplied_tikitt.discover_zone(7, 42, 441, 44101, 100, "arrival") ==
				result::applied,
			"Tikitzopl discovery fixture failed");
		journal = supplied_tikitt.render_journal(7, 42, 441, 10, 1, 101, false, false);
		require(journal.find("] " + treasure_key.title) == std::string::npos,
			"temple discovery exposed an unseen magician");
		for (const auto &contact : tikitt_map.contacts)
			require(supplied_tikitt.meet_npc(7, 42, contact.mob_vnum, 44101, 102) ==
					result::applied,
				"Tikitzopl encounter fixture failed");
		const auto tikitt_section = [&](const auto &story)
		{
			const auto at = journal.find("] " + story.title + "\r\n");
			require(at != std::string::npos, "Tikitzopl journal row missing");
			const auto end = journal.find("\r\n  [", at + 1);
			return journal.substr(at, end == std::string::npos ? end : end - at);
		};
		// Every recipe must consume its own exact currently carried kinds/counts;
		// optional access/producer history cannot block supplied ingredients.
		for (const auto &recipe : tikitt_map.stories)
		{
			supplies = {};
			for (const auto &step : recipe.steps)
				if (step.kind == "carried_item" && !step.optional)
					supplies.carried[step.item_vnums.front()] = step.count;
			const auto before_read = supplied_tikitt.serialize_state();
			journal = supplied_tikitt.render_journal(7, 42, 441, 10, 1, 125, false,
								 false, &supplies);
			require(tikitt_section(recipe).find("Next: " + recipe.steps.back().text) !=
						std::string::npos &&
					supplied_tikitt.serialize_state() == before_read,
				"supplied temple recipe required history or awarded an access/kill/quest event");
			for (const auto &step : recipe.steps)
			{
				if (step.kind != "carried_item" || step.optional)
					continue;
				const auto item = step.item_vnums.front();
				supplies.carried[item] = step.count - 1;
				supplies.equipped[16] = item;
				const auto other = std::find_if(
					recipe.steps.begin(), recipe.steps.end(),
					[&](const auto &candidate)
					{
						return candidate.kind == "carried_item" &&
						       !candidate.optional &&
						       candidate.item_vnums.front() != item;
					});
				if (other != recipe.steps.end())
					supplies.carried[other->item_vnums.front()] += 10;
				journal = supplied_tikitt.render_journal(7, 42, 441, 10, 1, 126,
									 false, false, &supplies);
				require(tikitt_section(recipe).find("Next: " + step.text) !=
						std::string::npos,
					"another ingredient or worn item replaced a missing exact carried root");
				if (other != recipe.steps.end())
					supplies.carried[other->item_vnums.front()] -= 10;
				supplies.equipped.clear();
				supplies.carried[item] = step.count;
			}
		}
		// Record selected producers, then spend their outputs. Receipt history is
		// preparation; it cannot substitute for a current base or consumed Orb.
		for (const auto *id : { "merge-five-stone-pieces", "merge-ten-standard-swords",
					"merge-four-guard-scimitars", "merge-five-claymores" })
			record(supplied_tikitt, story_for("tikitt", id).contracts.front(), id, 441,
			       44313);
		record(supplied_tikitt, treasure_key.contracts.front(), "tikitt-golden-key", 441,
		       44313);
		supplies = {};
		for (int item : { 44164, 44145, 44147, 44149, 43719 })
			supplies.carried[item] = 1;
		journal = supplied_tikitt.render_journal(7, 42, 441, 10, 1, 127, false, false,
							 &supplies);
		const auto missing_stone =
			std::find_if(mangler.steps.begin(), mangler.steps.end(),
				     [](const auto &step) {
					     return step.kind == "carried_item" &&
						    step.item_vnums.front() == 44143;
				     });
		require(missing_stone != mangler.steps.end() &&
				tikitt_section(mangler).find("Next: " + missing_stone->text) !=
					std::string::npos,
			"crafting history replaced the spent magical stone");
		supplies.carried[44143] = 1;
		supplies.carried.erase(44164);
		journal = supplied_tikitt.render_journal(7, 42, 441, 10, 1, 128, false, false,
							 &supplies);
		const auto missing_orb = std::find_if(mangler.steps.begin(), mangler.steps.end(),
						      [](const auto &step) {
							      return step.kind == "carried_item" &&
								     step.item_vnums.front() ==
									     44164;
						      });
		require(missing_orb != mangler.steps.end() &&
				tikitt_section(mangler).find("Next: " + missing_orb->text) !=
					std::string::npos,
			"golden-key history replaced a missing Orb or bypassed a competing recipe");
		service credited_tikitt(catalog);
		for (const auto &recipe : tikitt_map.stories)
			if (recipe.category == "service")
				record(credited_tikitt, recipe.contracts.front(), recipe.id.c_str(),
				       441, 44313);
		require(credited_tikitt.progress_for_zone(7, 42, 441).completed == 0 &&
				credited_tikitt.progress_for_zone(7, 42, 441).total == 4,
			"equipment services inflated temple story achievements");
		record(credited_tikitt, mirror.contracts.front(), "tikitt-supplied-mirror", 441,
		       44321);
		require(credited_tikitt.progress_for_zone(7, 42, 441).completed == 1,
			"supplied mirrored bracelet invented O'Gallagher's earlier request");
		for (const auto *story : { &clover, &zynar, &treasure_key })
			record(credited_tikitt, story->contracts.front(), story->id.c_str(), 441,
			       44313);
		service restored_tikitt(catalog);
		require(restored_tikitt.deserialize_state(credited_tikitt.serialize_state(),
							  &error) &&
				restored_tikitt.progress_for_zone(7, 42, 441).completed == 4 &&
				restored_tikitt.progress_for_zone(7, 42, 441).total == 4,
			"temple receipt recovery counted services or changed independent outcomes");
		const auto &jade_map =
			*std::find_if(catalog.story_mappings.begin(), catalog.story_mappings.end(),
				      [](const auto &m) { return m.source_area == "jade"; });
		const auto &jade_fish = story_for("jade", "one-fish-for-the-fisherman");
		const auto &jade_token = story_for("jade", "the-princesss-royal-token");
		service supplied_jade(catalog);
		require(supplied_jade.discover_zone(7, 42, 766, 76601, 100, "arrival") ==
				result::applied,
			"Jade discovery fixture failed");
		journal = supplied_jade.render_journal(7, 42, 766, 10, 1, 101, false, false);
		require(journal.find("] " + jade_token.title) == std::string::npos,
			"Jade discovery exposed an unseen Emperor request");
		for (const auto &contact : jade_map.contacts)
			require(supplied_jade.meet_npc(7, 42, contact.mob_vnum, 76601, 102) ==
					result::applied,
				"Jade encounter fixture failed");
		const auto jade_section = [&](const auto &story)
		{
			const auto at = journal.find("] " + story.title + "\r\n");
			require(at != std::string::npos, "Jade journal row missing");
			const auto end = journal.find("\r\n  [", at + 1);
			return journal.substr(at, end == std::string::npos ? end : end - at);
		};
		// Supplied proof skips optional sources and earlier exchanges. Exact live
		// quantities remain necessary, including four meat and two orchid roots.
		for (const auto &recipe : jade_map.stories)
		{
			supplies = {};
			for (const auto &step : recipe.steps)
				if (step.kind == "carried_item" && !step.optional)
					supplies.carried[step.item_vnums.front()] = step.count;
			const auto before_read = supplied_jade.serialize_state();
			journal = supplied_jade.render_journal(7, 42, 766, 10, 1, 125, false, false,
							       &supplies);
			require(jade_section(recipe).find("Next: " + recipe.steps.back().text) !=
						std::string::npos &&
					supplied_jade.serialize_state() == before_read,
				"supplied Jade proof required history or journal read awarded an event");
			for (const auto &step : recipe.steps)
			{
				if (step.optional)
					require(jade_section(recipe).find("Next: " + step.text) ==
							std::string::npos,
						"optional Jade access or producer displaced the delivery");
				if (step.kind != "carried_item" || step.optional)
					continue;
				const auto item = step.item_vnums.front();
				supplies.carried[item] = step.count - 1;
				supplies.equipped[16] = item;
				const auto other = std::find_if(
					recipe.steps.begin(), recipe.steps.end(),
					[&](const auto &candidate)
					{
						return candidate.kind == "carried_item" &&
						       !candidate.optional &&
						       candidate.item_vnums.front() != item;
					});
				if (other != recipe.steps.end())
					supplies.carried[other->item_vnums.front()] += 10;
				journal = supplied_jade.render_journal(7, 42, 766, 10, 1, 126,
								       false, false, &supplies);
				require(jade_section(recipe).find("Next: " + step.text) !=
						std::string::npos,
					"worn item or another Jade kind substituted for the missing quantity");
				if (other != recipe.steps.end())
					supplies.carried[other->item_vnums.front()] -= 10;
				supplies.equipped.clear();
				supplies.carried[item] = step.count;
			}
		}
		supplies = {};
		supplies.carried[319] = 1;
		journal = supplied_jade.render_journal(7, 42, 766, 10, 1, 127, false, false,
						       &supplies);
		require(jade_section(jade_fish).find("Next: " + jade_fish.steps.back().text) !=
				std::string::npos,
			"Jade fish alternative required both pike and lobster");
		fee_warnings = 0;
		for (size_t at = journal.find(unavailable); at != std::string::npos;
		     at = journal.find(unavailable, at + unavailable.size()))
			++fee_warnings;
		require(fee_warnings == 4 && jade_section(story_for("jade", "buy-a-capture-net"))
							     .find("still needs payment support") !=
						     std::string::npos,
			"Jade paid prerequisites lost warnings or cash rewards became unsupported fees");
		for (const char *id :
		     { "five-portions-for-a-harvest-bag", "grind-the-harvest-bag" })
			record(supplied_jade, story_for("jade", id).contracts.front(), id, 766,
			       76793);
		supplies = {};
		const auto &jade_hat = story_for("jade", "rice-paper-hat");
		journal = supplied_jade.render_journal(7, 42, 766, 10, 1, 128, false, false,
						       &supplies);
		const auto missing_rice =
			std::find_if(jade_hat.steps.begin(), jade_hat.steps.end(),
				     [](const auto &step)
				     { return step.kind == "carried_item" && !step.optional; });
		require(missing_rice != jade_hat.steps.end() &&
				jade_section(jade_hat).find("Next: " + missing_rice->text) !=
					std::string::npos,
			"rice producer receipts replaced a spent current portion");
		service credited_jade(catalog);
		for (const auto &recipe : jade_map.stories)
			if (recipe.category == "service")
				record(credited_jade, recipe.contracts.front(), recipe.id.c_str(),
				       766, 76601);
		for (const auto &excluded : jade_map.exclusions)
			record(credited_jade, excluded.first, excluded.first.c_str(), 766, 76601);
		require(credited_jade.progress_for_zone(7, 42, 766).completed == 0 &&
				credited_jade.progress_for_zone(7, 42, 766).total == 17,
			"Jade support exchanges or rejected/unfinished outcomes inflated achievements");
		record(credited_jade, jade_token.contracts.front(), "jade-supplied-token", 766,
		       76901);
		require(credited_jade.progress_for_zone(7, 42, 766).completed == 1,
			"supplied royal token invented invitation, rescue or other local history");
		for (const auto &recipe : jade_map.stories)
			if (recipe.category != "service")
				for (const auto &contract : recipe.contracts)
					record(credited_jade, contract, contract.c_str(), 766,
					       76601);
		service restored_jade(catalog);
		require(restored_jade.deserialize_state(credited_jade.serialize_state(), &error) &&
				restored_jade.progress_for_zone(7, 42, 766).completed == 17 &&
				restored_jade.progress_for_zone(7, 42, 766).total == 17,
			"Jade recovery counted equivalent fish, services or exclusions twice");
		const auto &savannah_map =
			*std::find_if(catalog.story_mappings.begin(), catalog.story_mappings.end(),
				      [](const auto &m) { return m.source_area == "savannah"; });
		const auto &epic_drums = story_for("savannah", "epic-drums");
		service supplied_savannah(catalog);
		require(supplied_savannah.discover_zone(7, 42, 1385, 138584, 100, "arrival") ==
				result::applied,
			"Savannah discovery fixture failed");
		journal = supplied_savannah.render_journal(7, 42, 1385, 10, 1, 101, false, false);
		require(journal.find("] " + epic_drums.title) == std::string::npos,
			"Savannah discovery exposed an unseen Lynstar service");
		for (const auto &contact : savannah_map.contacts)
			require(supplied_savannah.meet_npc(7, 42, contact.mob_vnum, 138584, 102) ==
					result::applied,
				"Savannah encounter fixture failed");
		const auto savannah_section = [&](const auto &story)
		{
			const auto at = journal.find("] " + story.title + "\r\n");
			require(at != std::string::npos, "Savannah journal row missing");
			const auto end = journal.find("\r\n  [", at + 1);
			return journal.substr(at, end == std::string::npos ? end : end - at);
		};
		// Supplied exact offerings skip source and preparation history. Current
		// quantities and matching instrument kinds still gate readiness.
		for (const auto &recipe : savannah_map.stories)
		{
			supplies = {};
			for (const auto &step : recipe.steps)
				if (step.kind == "carried_item" && !step.optional)
					supplies.carried[step.item_vnums.front()] = step.count;
			const auto before_read = supplied_savannah.serialize_state();
			journal = supplied_savannah.render_journal(7, 42, 1385, 10, 1, 125, false,
								   false, &supplies);
			require(savannah_section(recipe).find(
					"Next: " + recipe.steps.back().text) != std::string::npos &&
					supplied_savannah.serialize_state() == before_read,
				"supplied Savannah offering required history or a read awarded progress");
			for (const auto &step : recipe.steps)
			{
				if (step.optional)
				{
					require(savannah_section(recipe).find(
							"Next: " + step.text) == std::string::npos,
						"optional Savannah preparation became a required next step");
					continue;
				}
				if (step.kind != "carried_item")
					continue;
				const int item = step.item_vnums.front();
				supplies.carried[item] = step.count - 1;
				supplies.equipped[16] = item;
				journal = supplied_savannah.render_journal(7, 42, 1385, 10, 1, 126,
									   false, false, &supplies);
				require(savannah_section(recipe).find("Next: " + step.text) !=
						std::string::npos,
					"a worn item replaced a missing exact root or third animal part");
				supplies.equipped.clear();
				supplies.carried[item] = step.count;
			}
		}
		const auto kunji_history = std::find_if(
			epic_drums.steps.begin(), epic_drums.steps.end(),
			[](const auto &step) { return step.id == "retribution-preparation"; });
		require(kunji_history != epic_drums.steps.end() && kunji_history->optional,
			"optional foreign Kunji preparation missing");
		record(supplied_savannah, kunji_history->contracts.front(), "savannah-kunji", 1382,
		       138428);
		record(supplied_savannah,
		       story_for("savannah", "legendary-drums").contracts.front(),
		       "savannah-base-drums", 1385, 138665);
		supplies = {};
		supplies.carried[138279] = 1;
		supplies.carried[138535] = 1;
		journal = supplied_savannah.render_journal(7, 42, 1385, 10, 1, 127, false, false,
							   &supplies);
		const auto missing_drums =
			std::find_if(epic_drums.steps.begin(), epic_drums.steps.end(),
				     [](const auto &step) {
					     return step.kind == "carried_item" &&
						    step.item_vnums.front() == 138533;
				     });
		require(missing_drums != epic_drums.steps.end() &&
				savannah_section(epic_drums).find("Next: " + missing_drums->text) !=
					std::string::npos,
			"base history or a different instrument replaced the current matching drums");
		supplies.carried[138533] = 1;
		supplies.carried.erase(138279);
		journal = supplied_savannah.render_journal(7, 42, 1385, 10, 1, 128, false, false,
							   &supplies);
		const auto missing_sword =
			std::find_if(epic_drums.steps.begin(), epic_drums.steps.end(),
				     [](const auto &step) {
					     return step.kind == "carried_item" &&
						    step.item_vnums.front() == 138279;
				     });
		require(missing_sword != epic_drums.steps.end() &&
				savannah_section(epic_drums).find("Next: " + missing_sword->text) !=
					std::string::npos &&
				supplied_savannah.progress_for_zone(7, 42, 1385).completed == 0,
			"Kunji history replaced spent Retribution or credited a local collection");
		service credited_savannah(catalog);
		for (const auto &recipe : savannah_map.stories)
			if (recipe.category == "service")
				record(credited_savannah, recipe.contracts.front(),
				       recipe.id.c_str(), 1385, 138665);
		require(credited_savannah.progress_for_zone(7, 42, 1385).completed == 0 &&
				credited_savannah.progress_for_zone(7, 42, 1385).total == 5,
			"instrument trades invented collection or tribal achievements");
		for (const auto &recipe : savannah_map.stories)
			if (recipe.category != "service")
				record(credited_savannah, recipe.contracts.front(),
				       recipe.id.c_str(), 1385, 138584);
		service restored_savannah(catalog);
		require(restored_savannah.deserialize_state(credited_savannah.serialize_state(),
							    &error) &&
				restored_savannah.progress_for_zone(7, 42, 1385).completed == 5 &&
				restored_savannah.progress_for_zone(7, 42, 1385).total == 5,
			"Savannah receipt recovery merged named requests or counted gear services");
		const auto &realm_signet = story_for("realm", "finns-lost-signet");
		const auto &realm_key = story_for("realm", "finns-castle-key");
		const auto &realm_blade = story_for("realm", "celriyas-family-blade");
		const auto &realm_ring = story_for("realm", "finns-glowing-ring-trade");
		const auto &realm_forge = story_for("realm", "the-five-plane-forge-service");
		service supplied_realm(catalog);
		require(supplied_realm.discover_zone(7, 42, 140, 14024, 100, "arrival") ==
				result::applied,
			"Faerie discovery failed");
		for (int npc : { 14015, 14028, 14073, 14074 })
			require(supplied_realm.meet_npc(7, 42, npc, 14024, 101) == result::applied,
				"Faerie contact failed");
		supplies = {};
		supplies.carried[14037] = 1;
		const auto realm_before_view = supplied_realm.serialize_state();
		journal = supplied_realm.render_journal(7, 42, 140, 10, 1, 102, false, false,
							&supplies);
		const auto realm_section = [&](const auto &story)
		{
			const auto start = journal.find(story.title);
			require(start != std::string::npos,
				"Faerie story was hidden after meeting");
			const auto end = journal.find("\r\n  [", start + 1);
			return journal.substr(start, end == std::string::npos ? end : end - start);
		};
		require(realm_section(realm_key).find("Next: " + realm_key.steps.back().text) !=
					std::string::npos &&
				realm_section(realm_key).find("[Ready now] " +
							      realm_key.steps[2].text) !=
					std::string::npos &&
				supplied_realm.serialize_state() == realm_before_view &&
				supplied_realm.progress_for_zone(7, 42, 140).completed == 0,
			"supplied castle key required earlier ring/access history or a view wrote credit");
		record(supplied_realm, realm_key.contracts.front(), "realm-supplied-key", 140,
		       14024);
		require(supplied_realm.progress_for_zone(7, 42, 140).completed == 1 &&
				supplied_realm.progress_for_zone(7, 42, 140).total == 3,
			"Finn's retirement receipt completed other deliveries or counted services");
		supplies = {};
		for (int part : { 14121, 14122, 14123, 14124, 14125 })
			supplies.carried[part] = 1;
		for (const auto &step : realm_forge.steps)
		{
			if (step.kind != "carried_item")
				continue;
			const auto item = step.item_vnums.front();
			journal = supplied_realm.render_journal(7, 42, 140, 10, 1, 103, false,
								false, &supplies);
			require(realm_section(realm_forge).find("[Ready now] " + step.text) !=
					std::string::npos,
				"exact forge part was missing from current preparation");
			supplies.carried[item] = 0;
			supplies.carried[item == 14121 ? 14122 : 14121] += 5;
			supplies.equipped[16] = item;
			journal = supplied_realm.render_journal(7, 42, 140, 10, 1, 104, false,
								false, &supplies);
			require(realm_section(realm_forge).find("[Missing now] " + step.text) !=
					std::string::npos,
				"extra foreign part or worn material replaced an exact carried ingredient");
			supplies.equipped.clear();
			supplies.carried[item == 14121 ? 14122 : 14121] -= 5;
			supplies.carried[item] = 1;
		}
		record(supplied_realm, realm_ring.contracts.front(), "realm-ring-service", 140,
		       14024);
		for (size_t i = 0; i < realm_forge.contracts.size(); ++i)
			record(supplied_realm, realm_forge.contracts[i],
			       i == 0 ? "realm-jamfluul" : "realm-dopplepopper", 140, 14209);
		service restored_realm(catalog);
		require(restored_realm.deserialize_state(supplied_realm.serialize_state(),
							 &error) &&
				restored_realm.progress_for_zone(7, 42, 140).completed == 1 &&
				restored_realm.progress_for_zone(7, 42, 140).total == 3,
			"Faerie recovery counted equivalent maker/ring services as story outcomes");
		record(restored_realm, realm_signet.contracts.front(), "realm-later-signet", 140,
		       14024);
		require(restored_realm.progress_for_zone(7, 42, 140).completed == 2,
			"later independent signet was merged with the key finale");
		record(restored_realm, realm_blade.contracts.front(), "realm-celriya", 140, 14147);
		service recovered_realm(catalog);
		require(recovered_realm.deserialize_state(restored_realm.serialize_state(),
							  &error) &&
				recovered_realm.progress_for_zone(7, 42, 140).completed == 3 &&
				recovered_realm.progress_for_zone(7, 42, 140).total == 3,
			"Faerie cold state recovery lost independent delivery receipts");
		const auto &verspin_lion = story_for("verspin", "the-golden-lions-collar");
		const auto &verspin_bone = story_for("verspin", "ramous-apple-for-a-bone");
		const auto &verspin_totems = story_for("verspin", "tottans-five-totems");
		const auto &verspin_symbols = story_for("verspin", "the-shrines-five-symbols");
		const auto &verspin_amulets = story_for("verspin", "transos-three-amulets");
		const auto &verspin_monk = story_for("verspin", "the-monks-corruption-sigil");
		service supplied_verspin(catalog);
		require(supplied_verspin.discover_zone(7, 42, 281, 28100, 100, "arrival") ==
				result::applied,
			"Verspin discovery failed");
		for (const auto &[npc, room] : { std::pair{ 28116, 28150 },
						 { 28126, 28159 },
						 { 28128, 28159 },
						 { 28144, 28129 },
						 { 28145, 28103 },
						 { 28147, 28188 },
						 { 28155, 28212 },
						 { 28173, 28278 } })
			require(supplied_verspin.meet_npc(7, 42, npc, room, 101) == result::applied,
				"Verspin contact failed");
		supplies = {};
		supplies.carried[28113] = supplies.carried[28114] = 1;
		const auto verspin_before_view = supplied_verspin.serialize_state();
		journal = supplied_verspin.render_journal(7, 42, 281, 10, 1, 102, false, false,
							  &supplies);
		const auto verspin_section = [&](const auto &story)
		{
			const auto start = journal.find(story.title);
			require(start != std::string::npos,
				"Verspin story was hidden after meeting");
			const auto end = journal.find("\r\n  [", start + 1);
			return journal.substr(start, end == std::string::npos ? end : end - start);
		};
		require(verspin_section(verspin_lion)
						.find("Next: " + verspin_lion.steps.back().text) !=
					std::string::npos &&
				verspin_section(verspin_lion)
						.find("[Ready now] " +
						      verspin_lion.steps[2].text) !=
					std::string::npos &&
				verspin_section(verspin_lion)
						.find("[Recorded] " + verspin_lion.steps[1].text) ==
					std::string::npos &&
				supplied_verspin.serialize_state() == verspin_before_view &&
				supplied_verspin.progress_for_zone(7, 42, 281).completed == 0,
			"supplied bone required Ramous history or output possession/viewing granted credit");
		record(supplied_verspin, verspin_lion.contracts.front(), "verspin-supplied-bone",
		       281, 28159);
		require(supplied_verspin.progress_for_zone(7, 42, 281).completed == 1 &&
				supplied_verspin.progress_for_zone(7, 42, 281).total == 6,
			"lion receipt counted producer history or other independent outcomes");
		for (const auto *collection : { &verspin_totems, &verspin_symbols })
		{
			supplies = {};
			const auto &step = collection->steps.front();
			supplies.carried[step.item_vnums.front()] = 4;
			supplies.carried[collection == &verspin_totems ? 28138 : 28144] = 5;
			supplies.equipped[16] = step.item_vnums.front();
			journal = supplied_verspin.render_journal(7, 42, 281, 10, 1, 121, false,
								  false, &supplies);
			require(verspin_section(*collection).find("[Missing now] " + step.text) !=
					std::string::npos,
				"four carried proofs plus worn/other-kind stock replaced the five-count check");
			supplies.carried[step.item_vnums.front()] = 5;
			journal = supplied_verspin.render_journal(7, 42, 281, 10, 1, 122, false,
								  false, &supplies);
			require(verspin_section(*collection).find("[Ready now] " + step.text) !=
					std::string::npos,
				"five exact carried proofs did not satisfy optional preparation");
		}
		supplies = {};
		for (int item : { 28123, 28124, 28125 })
			supplies.carried[item] = 1;
		for (const auto &step : verspin_amulets.steps)
		{
			if (step.kind != "carried_item")
				continue;
			const auto item = step.item_vnums.front();
			journal = supplied_verspin.render_journal(7, 42, 281, 10, 1, 123, false,
								  false, &supplies);
			require(verspin_section(verspin_amulets).find("[Ready now] " + step.text) !=
					std::string::npos,
				"an exact amulet color was missing");
			supplies.carried[item] = 0;
			supplies.carried[item == 28123 ? 28124 : 28123] += 4;
			supplies.equipped[16] = item;
			journal = supplied_verspin.render_journal(7, 42, 281, 10, 1, 124, false,
								  false, &supplies);
			require(verspin_section(verspin_amulets).find("[Missing now] " + step.text) !=
					std::string::npos,
				"extra other-color or worn amulets replaced an exact carried color");
			supplies.equipped.clear();
			supplies.carried[item == 28123 ? 28124 : 28123] -= 4;
			supplies.carried[item] = 1;
		}
		supplies.carried[74298] = 1;
		journal = supplied_verspin.render_journal(7, 42, 281, 10, 1, 125, false, false,
							  &supplies);
		require(verspin_section(verspin_monk)
						.find("[Ready now] " +
						      verspin_monk.steps.front().text) !=
					std::string::npos &&
				supplied_verspin.progress_for_zone(7, 42, 281).completed == 1 &&
				supplied_verspin.progress_for_zone(7, 42, 550).completed == 0,
			"foreign sigil possession wrote a local or Bloodstone receipt");
		const auto &verspin_map = *std::find_if(
			catalog.story_mappings.begin(), catalog.story_mappings.end(),
			[](const auto &mapping) { return mapping.source_area == "verspin"; });
		for (const auto &entry : verspin_map.stories)
			if (entry.category == "service")
				record(supplied_verspin, entry.contracts.front(), entry.id.c_str(),
				       281, entry.id == verspin_bone.id ? 28159 : 28129);
		service restored_verspin(catalog);
		require(restored_verspin.deserialize_state(supplied_verspin.serialize_state(),
							   &error) &&
				restored_verspin.progress_for_zone(7, 42, 281).completed == 1 &&
				restored_verspin.progress_for_zone(7, 42, 281).total == 6,
			"Verspin recovery counted producer or paid equipment services");
		for (const auto &entry : verspin_map.stories)
			if (entry.category != "service" && entry.id != verspin_lion.id)
				record(restored_verspin, entry.contracts.front(), entry.id.c_str(),
				       281, entry.id == verspin_monk.id ? 28278 : 28150);
		service recovered_verspin(catalog);
		require(recovered_verspin.deserialize_state(restored_verspin.serialize_state(),
							    &error) &&
				recovered_verspin.progress_for_zone(7, 42, 281).completed == 6 &&
				recovered_verspin.progress_for_zone(7, 42, 281).total == 6 &&
				recovered_verspin.progress_for_zone(7, 42, 550).completed == 0,
			"Verspin recovery merged independent receipts or reassigned foreign proof ownership");
		// Port supplies and explanatory history cannot establish a personal journey.
		const auto &shipy_pol = story_for("shipy", "pols-lure-materials");
		const auto &shipy_potions = story_for("shipy", "bestiles-six-potions");
		const auto &shipy_crates = story_for("shipy", "grimashks-crate-recovery");
		const auto &shipy_chundel = story_for("shipy", "chundels-port-crates");
		const auto &shipy_map = *std::find_if(catalog.story_mappings.begin(),
						      catalog.story_mappings.end(),
						      [](const auto &mapping)
						      { return mapping.source_area == "shipy"; });
		service supplied_shipy(catalog);
		require(supplied_shipy.discover_zone(7, 42, 431, 43100, 100, "arrival") ==
					result::applied &&
				supplied_shipy.render_journal(7, 42, 431, 10, 1, 101, false, false)
						.find(shipy_pol.title) == std::string::npos,
			"port discovery exposed an unseen Pol story");
		for (const auto &contact : shipy_map.contacts)
			require(supplied_shipy.meet_npc(7, 42, contact.mob_vnum, 43116, 101) ==
					result::applied,
				"port contact could not be encountered");
		const auto shipy_section = [&](const auto &entry)
		{
			const auto start = journal.find(entry.title);
			require(start != std::string::npos, "port story was missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		supplies.carried[43137] = supplies.carried[43138] = 5;
		supplies.carried[43144] = 1;
		const auto shipy_before_view = supplied_shipy.serialize_state();
		journal = supplied_shipy.render_journal(7, 42, 431, 10, 1, 121, false, false,
							&supplies);
		require(shipy_section(shipy_pol).find("[Ready now] " + shipy_pol.steps[2].text) !=
					std::string::npos &&
				shipy_section(shipy_pol).find("[Ready now] " +
							      shipy_pol.steps[3].text) !=
					std::string::npos &&
				shipy_section(shipy_pol).find("[Recorded] " +
							      shipy_pol.steps[0].text) ==
					std::string::npos &&
				supplied_shipy.serialize_state() == shipy_before_view &&
				supplied_shipy.progress_for_zone(7, 42, 431).completed == 0,
			"supplied lure materials required the note or viewing/output possession granted credit");
		record(supplied_shipy, shipy_pol.contracts.front(), "shipy-supplied-lure", 431,
		       43116);
		require(supplied_shipy.progress_for_zone(7, 42, 431).completed == 1,
			"Pol required an earlier paid briefing receipt");
		// Every repeated preparation uses its exact carried count; worn and other
		// kinds cannot fill a shortage. Six potion colors are separate kinds.
		for (const auto &entry : shipy_map.stories)
			for (const auto &step : entry.steps)
			{
				if (step.kind != "carried_item" || step.item_vnums.size() != 1)
					continue;
				supplies = {};
				const int item = step.item_vnums.front();
				supplies.carried[item] = step.count - 1;
				supplies.carried[item == 43137 ? 43138 : 43137] = 10;
				supplies.equipped[16] = item;
				journal = supplied_shipy.render_journal(7, 42, 431, 10, 1, 122,
									false, false, &supplies);
				require(shipy_section(entry).find("[Missing now] " + step.text) !=
						std::string::npos,
					"wrong-kind or worn stock replaced an exact port supply");
				supplies.carried[item] = step.count;
				journal = supplied_shipy.render_journal(7, 42, 431, 10, 1, 123,
									false, false, &supplies);
				require(shipy_section(entry).find("[Ready now] " + step.text) !=
						std::string::npos,
					"an exact carried port supply was missing");
			}
		supplies = {};
		for (int item : { 2800, 9428, 11562, 40469, 66723, 93914 })
			supplies.carried[item] = 1;
		journal = supplied_shipy.render_journal(7, 42, 431, 10, 1, 124, false, false,
							&supplies);
		for (const auto &step : shipy_potions.steps)
			if (step.kind == "carried_item")
				require(shipy_section(shipy_potions)
							.find("[Ready now] " + step.text) !=
						std::string::npos,
					"the six distinct potion collection was incomplete");
		for (int item : { 43101, 43125 })
		{
			supplies = {};
			supplies.carried[item] = 1;
			journal = supplied_shipy.render_journal(7, 42, 431, 10, 1, 125, false,
								false, &supplies);
			require(shipy_section(shipy_crates)
						.find("[Ready now] " +
						      shipy_crates.steps.front().text) !=
					std::string::npos,
				"either legitimate crate kind failed the optional alternative check");
		}
		for (const auto &contract : shipy_crates.contracts)
			record(supplied_shipy, contract, contract.c_str(), 431, 43151);
		require(supplied_shipy.progress_for_zone(7, 42, 431).completed == 2,
			"Grimashk alternatives counted twice or completed Chundel");
		for (const auto &entry : shipy_map.stories)
			if (entry.category == "service")
				record(supplied_shipy, entry.contracts.front(), entry.id.c_str(),
				       431, 43311);
		service restored_shipy(catalog);
		require(restored_shipy.deserialize_state(supplied_shipy.serialize_state(),
							 &error) &&
				restored_shipy.progress_for_zone(7, 42, 431).completed == 2 &&
				restored_shipy.progress_for_zone(7, 42, 431).total == 19,
			"port services or alternate receipts inflated recovered completion");
		record(restored_shipy, shipy_chundel.contracts.front(), "shipy-chundel", 431,
		       43111);
		require(restored_shipy.progress_for_zone(7, 42, 431).completed == 3,
			"Chundel lost his independent delivery outcome");
		for (const auto &entry : shipy_map.stories)
			if (entry.category != "service")
				record(restored_shipy, entry.contracts.front(), entry.id.c_str(),
				       431, 43116);
		service recovered_shipy(catalog);
		require(recovered_shipy.deserialize_state(restored_shipy.serialize_state(),
							  &error) &&
				recovered_shipy.progress_for_zone(7, 42, 431).completed == 19 &&
				recovered_shipy.progress_for_zone(7, 42, 431).total == 19 &&
				recovered_shipy.progress_for_zone(7, 42, 490).completed == 0 &&
				recovered_shipy.progress_for_zone(7, 42, 943).completed == 0,
			"port recovery lost independent outcomes or assigned foreign proofs to source zones");
		// Competing souls and duplicate names need exact current kinds, not inferred history.
		const auto &cosmic_seal = story_for("cosmic", "four-soul-seal");
		const auto &cosmic_halo = story_for("cosmic", "zeeniums-soul-offering");
		const auto &cosmic_study = story_for("cosmic", "recovered-planetary-study");
		const auto &cosmic_wind = story_for("cosmic", "windwalkers-companion");
		const auto &cosmic_donations = story_for("cosmic", "other-soul-offerings");
		const auto &cosmic_map = *std::find_if(catalog.story_mappings.begin(),
						       catalog.story_mappings.end(),
						       [](const auto &mapping)
						       { return mapping.source_area == "cosmic"; });
		service supplied_cosmic(catalog);
		require(supplied_cosmic.discover_zone(7, 42, 760, 76001, 100, "arrival") ==
					result::applied &&
				supplied_cosmic.render_journal(7, 42, 760, 10, 1, 101, false, false)
						.find(cosmic_seal.title) == std::string::npos,
			"Ultarium discovery exposed an unseen box story");
		for (const auto &contact : cosmic_map.contacts)
			require(supplied_cosmic.meet_npc(7, 42, contact.mob_vnum, 76001, 101) ==
					result::applied,
				"Ultarium contact was not encountered");
		const auto cosmic_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Ultarium story was missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		supplies.carried[76069] = supplies.carried[76065] = supplies.carried[76032] = 1;
		const auto cosmic_before_view = supplied_cosmic.serialize_state();
		journal = supplied_cosmic.render_journal(7, 42, 760, 10, 1, 121, false, false,
							 &supplies);
		require(cosmic_section(cosmic_study)
						.find("[Missing now] " +
						      cosmic_study.steps.front().text) !=
					std::string::npos &&
				cosmic_section(cosmic_wind)
						.find("[Missing now] " +
						      cosmic_wind.steps.front().text) !=
					std::string::npos &&
				supplied_cosmic.serialize_state() == cosmic_before_view &&
				supplied_cosmic.progress_for_zone(7, 42, 760).completed == 0,
			"same-name outputs or journal inspection manufactured original proofs/history");
		for (const auto &entry : cosmic_map.stories)
			for (const auto &step : entry.steps)
			{
				if (step.kind != "carried_item" || step.item_vnums.size() != 1)
					continue;
				supplies = {};
				const int item = step.item_vnums.front();
				supplies.equipped[16] = item;
				journal = supplied_cosmic.render_journal(7, 42, 760, 10, 1, 122,
									 false, false, &supplies);
				require(cosmic_section(entry).find("[Missing now] " + step.text) !=
						std::string::npos,
					"worn stock replaced an exact carried Ultarium proof/key");
				supplies.carried[item] = 1;
				journal = supplied_cosmic.render_journal(7, 42, 760, 10, 1, 123,
									 false, false, &supplies);
				require(cosmic_section(entry).find("[Ready now] " + step.text) !=
						std::string::npos,
					"supplied exact Ultarium proof/key failed the current check");
			}
		supplies = {};
		supplies.carried[76038] = 4;
		journal = supplied_cosmic.render_journal(7, 42, 760, 10, 1, 124, false, false,
							 &supplies);
		for (const auto &step : cosmic_seal.steps)
			if (step.kind == "carried_item" && step.item_vnums.front() >= 76039 &&
			    step.item_vnums.front() <= 76041)
				require(cosmic_section(cosmic_seal)
							.find("[Missing now] " + step.text) !=
						std::string::npos,
					"four copies of one soul replaced four distinct council souls");
		for (const auto &contract : cosmic_donations.contracts)
			record(supplied_cosmic, contract, contract.c_str(), 760, 76145);
		supplies = {};
		journal = supplied_cosmic.render_journal(7, 42, 760, 10, 1, 125, false, false,
							 &supplies);
		for (const auto &step : cosmic_seal.steps)
			if (step.kind == "carried_item" && step.item_vnums.front() >= 76038 &&
			    step.item_vnums.front() <= 76041)
				require(cosmic_section(cosmic_seal)
							.find("[Missing now] " + step.text) !=
						std::string::npos,
					"earlier donations replaced current box proof");
		for (int item : { 76038, 76039, 76040, 76041 })
			supplies.carried[item] = 1;
		journal = supplied_cosmic.render_journal(7, 42, 760, 10, 1, 126, false, false,
							 &supplies);
		for (const auto &step : cosmic_seal.steps)
			if (step.kind == "carried_item" && step.item_vnums.front() >= 76038 &&
			    step.item_vnums.front() <= 76041)
				require(cosmic_section(cosmic_seal)
							.find("[Ready now] " + step.text) !=
						std::string::npos,
					"four distinct supplied souls did not prepare the box");
		record(supplied_cosmic, cosmic_seal.contracts.front(), "cosmic-supplied-seal", 760,
		       76228);
		require(supplied_cosmic.progress_for_zone(7, 42, 760).completed == 1 &&
				supplied_cosmic.progress_for_zone(7, 42, 760).total == 7,
			"box acceptance required an offering/key history or counted support donations");
		record(supplied_cosmic, cosmic_halo.contracts.front(), "cosmic-independent-halo",
		       760, 76145);
		for (const auto &entry : cosmic_map.stories)
			if (entry.category == "service" && entry.id != cosmic_donations.id)
				record(supplied_cosmic, entry.contracts.front(), entry.id.c_str(),
				       760, 76255);
		service restored_cosmic(catalog);
		require(restored_cosmic.deserialize_state(supplied_cosmic.serialize_state(),
							  &error) &&
				restored_cosmic.progress_for_zone(7, 42, 760).completed == 2,
			"Ultarium services inflated recovered story completion");
		const auto xavier_copy = std::find_if(
			catalog.definitions.begin(), catalog.definitions.end(),
			[](const auto &definition)
			{
				return definition.giver_vnum == 82507 &&
				       definition.completion_key ==
					       "676976653d493a37363036393b726563656976653d493a38323532323b6469736170706561723d30";
			});
		require(xavier_copy != catalog.definitions.end(),
			"foreign delivery-copy contract missing");
		record(restored_cosmic, xavier_copy->definition_id, "cosmic-foreign-copy", 825,
		       82550);
		require(restored_cosmic.progress_for_zone(7, 42, 760).completed == 2 &&
				restored_cosmic.progress_for_zone(7, 42, 825).completed == 1,
			"foreign study delivery fabricated local recovery or lost recipient ownership");
		for (const auto &entry : cosmic_map.stories)
			if (entry.category != "service" && entry.id != cosmic_seal.id &&
			    entry.id != cosmic_halo.id)
				record(restored_cosmic, entry.contracts.front(), entry.id.c_str(),
				       760, 76001);
		service recovered_cosmic(catalog);
		require(recovered_cosmic.deserialize_state(restored_cosmic.serialize_state(),
							   &error) &&
				recovered_cosmic.progress_for_zone(7, 42, 760).completed == 7 &&
				recovered_cosmic.progress_for_zone(7, 42, 760).total == 7 &&
				recovered_cosmic.progress_for_zone(7, 42, 825).completed == 1 &&
				recovered_cosmic.progress_for_zone(7, 42, 311).completed == 0 &&
				recovered_cosmic.progress_for_zone(7, 42, 766).completed == 0,
			"Ultarium recovery lost independent receipts or credited foreign proof sources");
		// Large-region stories retain exact materials and independent accepted receipts.
		const auto &surface_mystic = story_for("surface", "mystics-five-offerings");
		const auto &surface_nomad = story_for("surface", "nomads-white-potion");
		const auto &surface_hunter = story_for("surface", "hunters-grey-paw");
		const auto &surface_kres = story_for("surface", "kres-bait-market");
		const auto &surface_moldug = story_for("surface", "moldugs-food-market");
		const auto &surface_map = *std::find_if(
			catalog.story_mappings.begin(), catalog.story_mappings.end(),
			[](const auto &mapping) { return mapping.source_area == "surface"; });
		service supplied_surface(catalog);
		require(supplied_surface.discover_zone(7, 42, 5000, 500000, 100, "arrival") ==
					result::applied &&
				supplied_surface.render_journal(7, 42, 5000, 10, 1, 101, false,
								false)
						.find(surface_mystic.title) == std::string::npos,
			"Surface discovery exposed an unseen mystic request");
		for (const auto &contact : surface_map.contacts)
			require(supplied_surface.meet_npc(7, 42, contact.mob_vnum, 500000, 101) ==
					result::applied,
				"Surface contact was not encountered");
		const auto surface_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Surface story was missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		supplies.carried[500028] = 5;
		const auto surface_before_read = supplied_surface.serialize_state();
		journal = supplied_surface.render_journal(7, 42, 5000, 10, 1, 121, false, false,
							  &supplies);
		for (const auto &step : surface_mystic.steps)
			if (step.kind == "carried_item" && step.item_vnums.front() != 500028)
				require(surface_section(surface_mystic)
							.find("[Missing now] " + step.text) !=
						std::string::npos,
					"five fire lockets replaced five distinct mystic materials");
		require(supplied_surface.serialize_state() == surface_before_read &&
				supplied_surface.progress_for_zone(7, 42, 5000).completed == 0,
			"Surface preparation manufactured durable history");
		for (const auto &entry : surface_map.stories)
			for (const auto &step : entry.steps)
			{
				if (step.kind != "carried_item" || step.item_vnums.size() != 1)
					continue;
				supplies = {};
				const int item = step.item_vnums.front();
				supplies.equipped[16] = item;
				supplies.carried[item] = step.count - 1;
				journal = supplied_surface.render_journal(7, 42, 5000, 10, 1, 122,
									  false, false, &supplies);
				require(surface_section(entry).find("[Missing now] " + step.text) !=
						std::string::npos,
					"worn or insufficient Surface proof replaced exact carried quantity");
				supplies.carried[item] = step.count;
				journal = supplied_surface.render_journal(7, 42, 5000, 10, 1, 123,
									  false, false, &supplies);
				require(surface_section(entry).find("[Ready now] " + step.text) !=
						std::string::npos,
					"exact supplied Surface quantity failed current preparation");
			}
		for (const auto &[item, ready] :
		     { std::pair{ 2676, true }, std::pair{ 293, false } })
		{
			supplies = {};
			supplies.carried[item] = 1;
			journal = supplied_surface.render_journal(7, 42, 5000, 10, 1, 124, false,
								  false, &supplies);
			require(surface_section(surface_kres)
							.find(std::string(ready ? "[Ready now] " :
										  "[Missing now] ") +
							      surface_kres.steps.front().text) !=
						std::string::npos &&
					surface_section(surface_moldug)
							.find(std::string(ready ? "[Missing now] " :
										  "[Ready now] ") +
							      surface_moldug.steps.front().text) !=
						std::string::npos,
				"distinct Breale and normal bass kinds were treated as interchangeable");
		}
		record(supplied_surface, surface_nomad.contracts.front(), "surface-supplied-potion",
		       5000, 544219);
		supplies = {};
		journal = supplied_surface.render_journal(7, 42, 5000, 10, 1, 125, false, false,
							  &supplies);
		require(supplied_surface.progress_for_zone(7, 42, 5000).completed == 1 &&
				surface_section(surface_hunter)
						.find("[Missing now] " +
						      surface_hunter.steps.front().text) !=
					std::string::npos &&
				supplied_surface.progress_for_zone(7, 42, 831).completed == 0,
			"supplied potion delivery required or fabricated producer/source credit");
		for (const char *id : { "stargazers-air-locket", "philosophers-earth-locket",
					"travelers-water-locket", "enchanters-fire-locket" })
			record(supplied_surface, story_for("surface", id).contracts.front(), id,
			       5000, 619004);
		journal = supplied_surface.render_journal(7, 42, 5000, 10, 1, 126, false, false,
							  &supplies);
		for (const auto &step : surface_mystic.steps)
			if (step.kind == "carried_item")
				require(surface_section(surface_mystic)
							.find("[Missing now] " + step.text) !=
						std::string::npos,
					"consumed branch history substituted for current mystic materials");
		for (const auto &step : surface_mystic.steps)
			if (step.kind == "carried_item")
				supplies.carried[step.item_vnums.front()] = step.count;
		journal = supplied_surface.render_journal(7, 42, 5000, 10, 1, 127, false, false,
							  &supplies);
		for (const auto &step : surface_mystic.steps)
			if (step.kind == "carried_item")
				require(surface_section(surface_mystic)
							.find("[Ready now] " + step.text) !=
						std::string::npos,
					"five distinct supplied materials did not prepare the mystic");
		record(supplied_surface, surface_mystic.contracts.front(),
		       "surface-supplied-finale", 5000, 544842);
		const auto surface_before_services =
			supplied_surface.progress_for_zone(7, 42, 5000).completed;
		for (const auto &entry : surface_map.stories)
			if (entry.category == "service")
				for (const auto &contract : entry.contracts)
					record(supplied_surface, contract, contract.c_str(), 5000,
					       500000);
		require(supplied_surface.progress_for_zone(7, 42, 5000).completed ==
					surface_before_services &&
				supplied_surface.progress_for_zone(7, 42, 5000).total == 17,
			"Surface markets or guarded equipment receipts inflated story progress");
		service recovered_surface(catalog);
		require(recovered_surface.deserialize_state(supplied_surface.serialize_state(),
							    &error),
			"Surface independent receipt recovery failed");
		for (const auto &entry : surface_map.stories)
			if (entry.category != "service" && entry.id != surface_mystic.id &&
			    entry.id != surface_nomad.id &&
			    entry.id.find("-locket") == std::string::npos)
				record(recovered_surface, entry.contracts.front(), entry.id.c_str(),
				       5000, 500000);
		service restored_surface(catalog);
		require(restored_surface.deserialize_state(recovered_surface.serialize_state(),
							   &error) &&
				restored_surface.progress_for_zone(7, 42, 5000).completed == 17 &&
				restored_surface.progress_for_zone(7, 42, 5000).total == 17 &&
				restored_surface.progress_for_zone(7, 42, 431).completed == 0 &&
				restored_surface.progress_for_zone(7, 42, 262).completed == 0 &&
				restored_surface.progress_for_zone(7, 42, 831).completed == 0,
			"Surface recovery merged independent opposing requests or fabricated foreign source credit");
		// City preparation and supplied finales do not manufacture producer history.
		const auto &tharnadia_chiln = story_for("tharnadia", "chilns-medicine-and-pendant");
		const auto &tharnadia_toys = story_for("tharnadia", "arkelyns-three-toys");
		const auto &tharnadia_medicine = story_for("tharnadia", "nebbles-medicine-service");
		const auto &tharnadia_sword =
			story_for("tharnadia", "zechs-two-handed-sword-service");
		const auto &tharnadia_paper = story_for("tharnadia", "ithilins-paper-map-service");
		const auto &tharnadia_map = *std::find_if(
			catalog.story_mappings.begin(), catalog.story_mappings.end(),
			[](const auto &mapping) { return mapping.source_area == "tharnadia"; });
		service supplied_tharnadia(catalog);
		require(supplied_tharnadia.discover_zone(7, 42, 1325, 132573, 100, "arrival") ==
					result::applied &&
				supplied_tharnadia
						.render_journal(7, 42, 1325, 10, 1, 101, false,
								false)
						.find(tharnadia_chiln.title) == std::string::npos,
			"Tharnadia discovery revealed an unseen named request");
		for (const auto &contact : tharnadia_map.contacts)
			require(supplied_tharnadia.meet_npc(7, 42, contact.mob_vnum, 132573, 101) ==
					result::applied,
				"Tharnadia contact encounter failed");
		const auto tharnadia_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Tharnadia story was missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		for (int item : { 132689, 132690, 132691, 132692, 132693, 132704 })
			supplies.carried[item] = 1;
		const auto tharnadia_before_read = supplied_tharnadia.serialize_state();
		journal = supplied_tharnadia.render_journal(7, 42, 1325, 10, 1, 121, false, false,
							    &supplies);
		for (const auto &step : tharnadia_toys.steps)
			if (step.kind == "carried_item")
				require(tharnadia_section(tharnadia_toys)
							.find("[Missing now] " + step.text) !=
						std::string::npos,
					"borrowed instruments substituted for the children's toy proofs");
		require(supplied_tharnadia.serialize_state() == tharnadia_before_read &&
				supplied_tharnadia.progress_for_zone(7, 42, 1325).completed == 0,
			"Tharnadia inventory inspection manufactured history");
		supplies = {};
		supplies.carried[132682] = 3;
		journal = supplied_tharnadia.render_journal(7, 42, 1325, 10, 1, 122, false, false,
							    &supplies);
		for (const auto &step : tharnadia_toys.steps)
			if (step.kind == "carried_item" && step.item_vnums.front() != 132682)
				require(tharnadia_section(tharnadia_toys)
							.find("[Missing now] " + step.text) !=
						std::string::npos,
					"three flutes replaced three different toy instruments");
		for (const auto &entry : tharnadia_map.stories)
			for (const auto &step : entry.steps)
			{
				if (step.kind != "carried_item")
					continue;
				for (int item : step.item_vnums)
				{
					supplies = {};
					supplies.equipped[16] = item;
					journal = supplied_tharnadia.render_journal(
						7, 42, 1325, 10, 1, 123, false, false, &supplies);
					require(tharnadia_section(entry).find("[Missing now] " +
									      step.text) !=
							std::string::npos,
						"equipped city proof substituted for a carried offering");
					supplies.equipped.clear();
					supplies.carried[item] = 1;
					journal = supplied_tharnadia.render_journal(
						7, 42, 1325, 10, 1, 124, false, false, &supplies);
					require(tharnadia_section(entry).find("[Ready now] " +
									      step.text) !=
							std::string::npos,
						"exact supplied city proof or alternative failed readiness");
				}
			}
		supplies = {};
		supplies.carried[132697] = supplies.carried[132699] = 1;
		journal = supplied_tharnadia.render_journal(7, 42, 1325, 10, 1, 125, false, false,
							    &supplies);
		require(tharnadia_section(tharnadia_chiln)
					.find("[Pending] " + tharnadia_chiln.steps.front().text) !=
				std::string::npos,
			"supplied vial invented earlier medicine production");
		record(supplied_tharnadia, tharnadia_chiln.contracts.front(),
		       "tharnadia-supplied-finale", 1325, 132833);
		require(supplied_tharnadia.progress_for_zone(7, 42, 1325).completed == 1,
			"supplied city finale required personal herb recovery");
		for (const auto &entry : tharnadia_map.stories)
			if (entry.category == "service")
				for (const auto &contract : entry.contracts)
					record(supplied_tharnadia, contract, contract.c_str(), 1325,
					       132573);
		supplies = {};
		journal = supplied_tharnadia.render_journal(7, 42, 1325, 10, 1, 126, false, false,
							    &supplies);
		require(supplied_tharnadia.progress_for_zone(7, 42, 1325).completed == 1 &&
				supplied_tharnadia.progress_for_zone(7, 42, 1325).total == 8 &&
				tharnadia_section(tharnadia_chiln)
						.find("[Recorded] " +
						      tharnadia_chiln.steps.front().text) !=
					std::string::npos &&
				tharnadia_section(tharnadia_medicine)
						.find("[Missing now] " +
						      tharnadia_medicine.steps.front().text) !=
					std::string::npos &&
				tharnadia_section(tharnadia_sword)
						.find("[Missing now] " +
						      tharnadia_sword.steps.front().text) !=
					std::string::npos &&
				tharnadia_section(tharnadia_paper)
						.find("[Missing now] " +
						      tharnadia_paper.steps.front().text) !=
					std::string::npos,
			"city services inflated achievements or replaced consumed supplies with history");
		service recovered_tharnadia(catalog);
		require(recovered_tharnadia.deserialize_state(supplied_tharnadia.serialize_state(),
							      &error),
			"Tharnadia receipt recovery failed");
		for (const auto &entry : tharnadia_map.stories)
			if (entry.category != "service" && entry.id != tharnadia_chiln.id)
				record(recovered_tharnadia, entry.contracts.front(),
				       entry.id.c_str(), 1325, 132573);
		service restored_tharnadia(catalog);
		require(restored_tharnadia.deserialize_state(recovered_tharnadia.serialize_state(),
							     &error) &&
				restored_tharnadia.progress_for_zone(7, 42, 1325).completed == 8 &&
				restored_tharnadia.progress_for_zone(7, 42, 1325).total == 8 &&
				restored_tharnadia.progress_for_zone(7, 42, 989).completed == 0 &&
				restored_tharnadia.progress_for_zone(7, 42, 292).completed == 0,
			"city recovery merged requests or fabricated foreign recovery credit");
		const auto &mini_sword = story_for("minizones", "restore-magik");
		const auto &mini_knight = story_for("minizones", "release-worach");
		const auto &mini_tips = story_for("minizones", "tips-for-the-dishwasher");
		service supplied_mini(catalog);
		require(supplied_mini.discover_zone(7, 42, 57, 5868, 100, "arrival") ==
					result::applied &&
				supplied_mini.meet_npc(7, 42, 5801, 5904, 101) == result::applied,
			"Mini Zones arrival and wise-man encounter failed");
		const auto mini_section = [&](const std::string &view, const auto &entry)
		{
			const auto start = view.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Mini Zones story section missing");
			const auto end = view.find("\r\n  [", start);
			return view.substr(start, end == std::string::npos ? end : end - start);
		};
		supplies.carried.clear();
		supplies.equipped.clear();
		supplies.carried[5793] = 3;
		journal = supplied_mini.render_journal(7, 42, 57, 10, 1, 102, false, false,
						       &supplies);
		require(mini_section(journal, mini_sword)
						.find("[Missing now] " +
						      mini_sword.steps[3].text) !=
					std::string::npos &&
				mini_section(journal, mini_sword)
						.find("[Missing now] " +
						      mini_sword.steps[4].text) !=
					std::string::npos,
			"duplicate metal strips replaced distinct sword kinds");
		supplies.carried[5793] = supplies.carried[5794] = supplies.carried[5795] = 1;
		supplies.equipped[16] = 5806;
		journal = supplied_mini.render_journal(7, 42, 57, 10, 1, 103, false, false,
						       &supplies);
		require(mini_section(journal, mini_sword)
					.find("[Missing now] " + mini_sword.steps[5].text) !=
				std::string::npos,
			"equipped hilt counted as a carried offering");
		supplies.equipped.clear();
		supplies.carried[5806] = 1;
		const auto mini_before_read = supplied_mini.serialize_state();
		journal = supplied_mini.render_journal(7, 42, 57, 10, 1, 104, false, false,
						       &supplies);
		for (size_t i = 2; i < 6; ++i)
			require(mini_section(journal, mini_sword)
						.find("[Ready now] " + mini_sword.steps[i].text) !=
					std::string::npos,
				"supplied exact sword material was not ready");
		require(supplied_mini.serialize_state() == mini_before_read &&
				mini_section(journal, mini_sword)
						.find("Next: " + mini_sword.steps.back().text) !=
					std::string::npos &&
				supplied_mini.progress_for_zone(7, 42, 57).completed == 0,
			"supplied sword materials invented history or blocked the finale");
		record(supplied_mini, mini_sword.contracts.front(), "mini-supplied-sword", 57,
		       5904);
		require(supplied_mini.meet_npc(7, 42, 5813, 5989, 105) == result::applied,
			"Mini Zones battlemaster encounter failed");
		const auto &mini_armor = story_for("minizones", "thrulmar-armplates");
		supplies.carried.clear();
		supplies.carried[5811] = 1;
		supplies.carried[500025] = 4;
		journal = supplied_mini.render_journal(7, 42, 57, 10, 1, 106, false, false,
						       &supplies);
		require(mini_section(journal, mini_armor)
					.find("[Missing now] " + mini_armor.steps[1].text) !=
				std::string::npos,
			"four blood crystals satisfied a five-crystal service");
		supplies.carried[500025] = 5;
		const auto mini_before_service_read = supplied_mini.serialize_state();
		journal = supplied_mini.render_journal(7, 42, 57, 10, 1, 107, false, false,
						       &supplies);
		require(mini_section(journal, mini_armor)
						.find("[Ready now] " + mini_armor.steps[1].text) !=
					std::string::npos &&
				supplied_mini.serialize_state() == mini_before_service_read &&
				supplied_mini.progress_for_zone(7, 42, 57).completed == 1,
			"prepared armor materials mutated history or earned a quest outcome");
		for (const char *id : { "thrulmar-armplates", "thrulmar-legplates",
					"thrulmar-gloves", "thrulmar-boots" })
		{
			const auto &recipe = story_for("minizones", id);
			require(recipe.category == "service" && recipe.steps[1].count == 5 &&
					recipe.steps[1].item_vnums ==
						std::vector<int32_t>{ 500025 },
				"Mini Zones recipe lost its exact crystal quantity or service role");
			record(supplied_mini, recipe.contracts.front(), id, 57, 5989);
		}
		require(supplied_mini.progress_for_zone(7, 42, 57).completed == 1 &&
				supplied_mini.progress_for_zone(7, 42, 57).total == 3,
			"armor services inflated Mini Zones outcomes");
		service restored_mini(catalog);
		require(restored_mini.deserialize_state(supplied_mini.serialize_state(), &error) &&
				restored_mini.progress_for_zone(7, 42, 57).completed == 1,
			"supplied Mini Zones finale lost its independent receipt on recovery");
		record(restored_mini, mini_knight.contracts.front(), "mini-knight", 57, 5892);
		record(restored_mini, mini_tips.contracts.front(), "mini-tips", 57, 5845);
		service recovered_mini(catalog);
		require(recovered_mini.deserialize_state(restored_mini.serialize_state(), &error) &&
				recovered_mini.progress_for_zone(7, 42, 57).completed == 3 &&
				recovered_mini.progress_for_zone(7, 42, 57).total == 3 &&
				recovered_mini.progress_for_zone(7, 42, 5000).completed == 0,
			"independent Mini Zones outcomes or foreign ownership failed recovery");
		const auto &torrhan_map = *std::find_if(
			catalog.story_mappings.begin(), catalog.story_mappings.end(),
			[](const auto &mapping) { return mapping.source_area == "torrhan"; });
		const auto &torrhan_king = story_for("torrhan", "king-torrhans-potion");
		const auto &torrhan_owl = story_for("torrhan", "owl-ladys-yellow-potion");
		const auto &torrhan_torrok = story_for("torrhan", "torroks-restored-oblivion");
		service supplied_torrhan(catalog);
		require(supplied_torrhan.discover_zone(7, 42, 666, 66600, 100, "arrival") ==
					result::applied &&
				supplied_torrhan.render_journal(7, 42, 666, 10, 1, 101, false,
								false)
						.find(torrhan_king.title) == std::string::npos,
			"Torrhan discovery revealed an unseen royal story");
		for (const auto &contact : torrhan_map.contacts)
			require(supplied_torrhan.meet_npc(7, 42, contact.mob_vnum, 66600, 101) ==
					result::applied,
				"Torrhan contact encounter failed");
		const auto torrhan_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Torrhan journal section missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		supplies.carried[66666] = 1;
		const auto torrhan_before_read = supplied_torrhan.serialize_state();
		journal = supplied_torrhan.render_journal(7, 42, 666, 10, 1, 121, false, false,
							  &supplies);
		require(torrhan_section(torrhan_king)
						.find("[Missing now] " +
						      torrhan_king.steps[2].text) !=
					std::string::npos &&
				torrhan_section(torrhan_owl)
						.find("[Ready now] " + torrhan_owl.steps[1].text) !=
					std::string::npos &&
				torrhan_section(torrhan_king)
						.find("[Pending] " + torrhan_king.steps[0].text) !=
					std::string::npos,
			"full yellow potion replaced half-empty material or invented owl history");
		supplies = {};
		supplies.carried[66673] = supplies.carried[66606] = supplies.carried[66652] = 1;
		journal = supplied_torrhan.render_journal(7, 42, 666, 10, 1, 122, false, false,
							  &supplies);
		require(torrhan_section(torrhan_king)
						.find("[Ready now] " +
						      torrhan_king.steps[2].text) !=
					std::string::npos &&
				torrhan_section(torrhan_owl)
						.find("[Missing now] " +
						      torrhan_owl.steps[1].text) !=
					std::string::npos &&
				torrhan_section(torrhan_torrok)
						.find("[Missing now] " +
						      torrhan_torrok.steps[1].text) !=
					std::string::npos &&
				torrhan_section(torrhan_torrok)
						.find("[Missing now] " +
						      torrhan_torrok.steps[2].text) !=
					std::string::npos,
			"Torrhan same-named or intermediate items substituted for exact offerings");
		supplies = {};
		supplies.carried[66639] = 1;
		journal = supplied_torrhan.render_journal(7, 42, 666, 10, 1, 123, false, false,
							  &supplies);
		for (int form = 1; form <= 8; ++form)
		{
			const auto id = "cloak-form-" + std::to_string(form);
			const auto &recipe = story_for("torrhan", id.c_str());
			require(recipe.category == "service" &&
					torrhan_section(recipe).find(
						std::string(form == 2 ? "[Ready now] " :
									"[Missing now] ") +
						recipe.steps.front().text) != std::string::npos,
				"same-named cloaks lost exact form readiness or service classification");
		}
		require(supplied_torrhan.serialize_state() == torrhan_before_read &&
				supplied_torrhan.progress_for_zone(7, 42, 666).completed == 0,
			"Torrhan inventory inspection manufactured quest history");
		record(supplied_torrhan, torrhan_king.contracts.front(), "torrhan-supplied-king",
		       666, 66801);
		record(supplied_torrhan, torrhan_torrok.contracts.front(), "torrhan-supplied-sword",
		       666, 66831);
		for (const auto &entry : torrhan_map.stories)
			if (entry.category == "service")
				for (const auto &contract : entry.contracts)
					record(supplied_torrhan, contract, contract.c_str(), 666,
					       66816);
		for (const auto &[contract, reason] : torrhan_map.exclusions)
			record(supplied_torrhan, contract, contract.c_str(), 666, 66721);
		require(supplied_torrhan.progress_for_zone(7, 42, 666).completed == 2 &&
				supplied_torrhan.progress_for_zone(7, 42, 666).total == 8,
			"supplied finales required producer history or services/refusals earned credit");
		service restored_torrhan(catalog);
		require(restored_torrhan.deserialize_state(supplied_torrhan.serialize_state(),
							   &error),
			"Torrhan supplied-finale recovery failed");
		for (const auto &entry : torrhan_map.stories)
			if (entry.category != "service" && entry.id != torrhan_king.id &&
			    entry.id != torrhan_torrok.id)
				record(restored_torrhan, entry.contracts.front(), entry.id.c_str(),
				       666, 66600);
		service recovered_torrhan(catalog);
		require(recovered_torrhan.deserialize_state(restored_torrhan.serialize_state(),
							    &error) &&
				recovered_torrhan.progress_for_zone(7, 42, 666).completed == 8 &&
				recovered_torrhan.progress_for_zone(7, 42, 666).total == 8 &&
				recovered_torrhan.progress_for_zone(7, 42, 5000).completed == 0 &&
				recovered_torrhan.progress_for_zone(7, 42, 57).completed == 0,
			"Torrhan recovery merged independent outcomes or invented foreign credit");

		const auto &gold_map = *std::find_if(catalog.story_mappings.begin(),
						     catalog.story_mappings.end(),
						     [](const auto &mapping)
						     { return mapping.source_area == "gold_hal"; });
		const auto &gold_finale = story_for("gold_hal", "three-proofs-for-wasephius");
		const auto &gold_trainer = story_for("gold_hal", "tields-stolen-amulet");
		const auto &gold_note = story_for("gold_hal", "the-bloodstained-note");
		const auto &gold_kenku = story_for("gold_hal", "release-the-kenku");
		service supplied_gold(catalog);
		require(supplied_gold.discover_zone(7, 42, 404, 40400, 100, "arrival") ==
					result::applied &&
				supplied_gold.render_journal(7, 42, 404, 10, 1, 101, false, false)
						.find(gold_finale.title) == std::string::npos,
			"Golden Hall discovery revealed an unseen royal story");
		for (const auto &contact : gold_map.contacts)
			require(supplied_gold.meet_npc(7, 42, contact.mob_vnum, 40400, 101) ==
					result::applied,
				"Golden Hall contact encounter failed");
		const auto gold_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Golden Hall journal section missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		supplies.carried[40403] = supplies.carried[40459] = supplies.carried[40498] = 1;
		supplies.carried[40463] = 1;
		const auto gold_before_read = supplied_gold.serialize_state();
		journal = supplied_gold.render_journal(7, 42, 404, 10, 1, 102, false, false,
						       &supplies);
		require(gold_section(gold_note).find("[Missing now] " + gold_note.steps[0].text) !=
					std::string::npos &&
				gold_section(gold_trainer)
						.find("[Missing now] " +
						      gold_trainer.steps[1].text) !=
					std::string::npos &&
				gold_section(gold_finale)
						.find("[Missing now] " +
						      gold_finale.steps[5].text) !=
					std::string::npos &&
				gold_section(gold_kenku)
						.find("[Missing now] " +
						      gold_kenku.steps[0].text) !=
					std::string::npos,
			"Golden Hall wrong note, totem, sword or silver key replaced exact proof");
		supplies = {};
		supplies.carried[40495] = supplies.carried[40483] = supplies.carried[40465] = 1;
		journal = supplied_gold.render_journal(7, 42, 404, 10, 1, 103, false, false,
						       &supplies);
		for (int index : { 4, 5, 6 })
			require(gold_section(gold_finale)
						.find("[Ready now] " +
						      gold_finale.steps[index].text) !=
					std::string::npos,
				"supplied Golden Hall final proof failed readiness");
		for (int index : { 0, 1, 2 })
			require(gold_section(gold_finale)
						.find("[Pending] " +
						      gold_finale.steps[index].text) !=
					std::string::npos,
				"supplied Golden Hall proof manufactured producer history");
		require(supplied_gold.serialize_state() == gold_before_read &&
				supplied_gold.progress_for_zone(7, 42, 404).completed == 0,
			"Golden Hall read manufactured quest completion");
		record(supplied_gold, gold_finale.contracts.front(), "gold-supplied-finale", 404,
		       40498);
		for (const auto &entry : gold_map.stories)
			if (entry.category == "service")
				record(supplied_gold, entry.contracts.front(), entry.id.c_str(),
				       404, 40400);
		require(supplied_gold.progress_for_zone(7, 42, 404).completed == 1 &&
				supplied_gold.progress_for_zone(7, 42, 404).total == 9,
			"Golden Hall finale required predecessors or services earned achievements");
		for (const char *id : { "release-the-half-elf", "release-the-kenku" })
		{
			const auto &rescue = story_for("gold_hal", id);
			const auto definition = std::find_if(
				catalog.definitions.begin(), catalog.definitions.end(),
				[&](const auto &d)
				{ return d.definition_id == rescue.contracts.front(); });
			require(definition != catalog.definitions.end() &&
					definition->eligible_for_zone_completion &&
					!definition->daily_eligible &&
					definition->daily_exclusion == "Item exchange",
				"key-return rescue lost achievement eligibility or acquired daily credit");
			record(supplied_gold, rescue.contracts.front(), rescue.id.c_str(), 404,
			       40635);
		}
		require(supplied_gold.progress_for_zone(7, 42, 404).completed == 3,
			"Golden Hall key-return rescues were treated as refusals");
		service restored_gold(catalog);
		require(restored_gold.deserialize_state(supplied_gold.serialize_state(), &error),
			"Golden Hall supplied-finale and rescue recovery failed");
		for (const auto &entry : gold_map.stories)
			if (entry.category != "service" && entry.id != gold_finale.id &&
			    entry.id != story_for("gold_hal", "release-the-half-elf").id &&
			    entry.id != gold_kenku.id)
				record(restored_gold, entry.contracts.front(), entry.id.c_str(),
				       404, 40400);
		service recovered_gold(catalog);
		require(recovered_gold.deserialize_state(restored_gold.serialize_state(), &error) &&
				recovered_gold.progress_for_zone(7, 42, 404).completed == 9 &&
				recovered_gold.progress_for_zone(7, 42, 404).total == 9 &&
				recovered_gold.progress_for_zone(7, 42, 5000).completed == 0 &&
				recovered_gold.progress_for_zone(7, 42, 57).completed == 0,
			"Golden Hall recovery merged independent stories or invented foreign credit");

		const auto &ash_map = *std::find_if(catalog.story_mappings.begin(),
						    catalog.story_mappings.end(),
						    [](const auto &mapping)
						    { return mapping.source_area == "ashrumite"; });
		const auto &ash_five = story_for("ashrumite", "silversmith-five-raw-gems");
		const auto &ash_necklace = story_for("ashrumite", "jeweler-current-necklace");
		const auto &ash_mage = story_for("ashrumite", "mage-current-necklace-enchantment");
		service supplied_ash(catalog);
		require(supplied_ash.discover_zone(7, 42, 660, 66001, 100, "arrival") ==
					result::applied &&
				supplied_ash.render_journal(7, 42, 660, 10, 1, 101, false, false)
						.find("] " + ash_five.title + "\r\n") ==
					std::string::npos,
			"Ashrumite discovery revealed an unseen crafting service");
		for (const auto &contact : ash_map.contacts)
			require(supplied_ash.meet_npc(7, 42, contact.mob_vnum, 66001, 101) ==
					result::applied,
				"Ashrumite fixture encounter failed");
		const auto ash_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Ashrumite journal section missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		supplies.carried[66032] = 5;
		supplies.carried[66033] = 4;
		supplies.carried[66048] = 2;
		supplies.carried[66051] = 1;
		const auto ash_before_read = supplied_ash.serialize_state();
		journal = supplied_ash.render_journal(7, 42, 660, 10, 1, 102, false, false,
						      &supplies);
		require(ash_section(ash_five).find("[Missing now] " + ash_five.steps[1].text) !=
					std::string::npos &&
				ash_section(ash_necklace)
						.find("[Missing now] " +
						      ash_necklace.steps[1].text) !=
					std::string::npos &&
				ash_section(ash_mage).find("[Missing now] " +
							   ash_mage.steps[1].text) !=
					std::string::npos,
			"same-name gems, ordinary necklace or pyrite replaced exact Ashrumite materials");
		supplies.carried[66033] = 5;
		for (int item : { 66049, 66044, 66045, 66046, 66047, 66050 })
			supplies.carried[item] = 1;
		journal = supplied_ash.render_journal(7, 42, 660, 10, 1, 103, false, false,
						      &supplies);
		require(ash_section(ash_five).find("[Ready now] " + ash_five.steps[1].text) !=
					std::string::npos &&
				ash_section(ash_five).find("[Pending] " + ash_five.steps[0].text) !=
					std::string::npos &&
				ash_section(ash_mage).find("[Ready now] " +
							   ash_mage.steps[1].text) !=
					std::string::npos &&
				ash_section(ash_mage).find("unavailable disc") != std::string::npos,
			"Ashrumite supplied material manufactured producer history or hid the missing disc");
		fee_warnings = 0;
		for (size_t at = journal.find(unavailable); at != std::string::npos;
		     at = journal.find(unavailable, at + unavailable.size()))
			++fee_warnings;
		require(fee_warnings == 11 &&
				ash_section(story_for("ashrumite", "bartenders-paid-disc-rumor"))
						.find("guarded under active accounting") !=
					std::string::npos &&
				supplied_ash.serialize_state() == ash_before_read &&
				supplied_ash.progress_for_zone(7, 42, 660).completed == 0 &&
				supplied_ash.progress_for_zone(7, 42, 660).total == 0,
			"Ashrumite guidance bypassed a payment guard, mutated state or counted a service");
		// Recovered historical receipts only: this fixture does not execute paid crafting.
		for (const auto &entry : ash_map.stories)
			record(supplied_ash, entry.contracts.front(), entry.id.c_str(), 660, 66060);
		service recovered_ash(catalog);
		require(recovered_ash.deserialize_state(supplied_ash.serialize_state(), &error) &&
				recovered_ash.progress_for_zone(7, 42, 660).completed == 0 &&
				recovered_ash.progress_for_zone(7, 42, 660).total == 0 &&
				recovered_ash.progress_for_zone(7, 42, 40).completed == 0 &&
				recovered_ash.progress_for_zone(7, 42, 252).completed == 0,
			"Ashrumite recovered services earned local or foreign quest credit");

		const auto &hall_map = *std::find_if(catalog.story_mappings.begin(),
						     catalog.story_mappings.end(),
						     [](const auto &mapping)
						     { return mapping.source_area == "hall"; });
		const auto &hall_belt = story_for("hall", "jadem-sixteen-part-device");
		const auto &hall_armor = story_for("hall", "xamael-platemail-of-awe");
		const auto &hall_child = story_for("hall", "child-dagger-for-letter");
		const auto &hall_letter = story_for("hall", "lost-aberrate-letter-for-hair");
		service supplied_hall(catalog);
		require(supplied_hall.discover_zone(7, 42, 777, 77700, 100, "arrival") ==
					result::applied &&
				supplied_hall.render_journal(7, 42, 777, 10, 1, 101, false, false)
						.find("] " + hall_armor.title + "\r\n") ==
					std::string::npos,
			"Hall discovery revealed an unmet smith commission");
		for (const auto &contact : hall_map.contacts)
			require(supplied_hall.meet_npc(7, 42, contact.mob_vnum, 77700, 101) ==
					result::applied,
				"Hall fixture encounter failed");
		const auto hall_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Hall journal section missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		supplies.carried[77744] = 2;
		supplies.carried[77712] = 1;
		supplies.carried[77729] = 2;
		supplies.carried[77719] = 2;
		supplies.carried[77742] = 4;
		supplies.carried[77748] = 5;
		supplies.carried[77726] = 1;
		supplies.equipped[16] = 18309;
		const auto hall_before = supplied_hall.serialize_state();
		journal = supplied_hall.render_journal(7, 42, 777, 10, 1, 102, false, false,
						       &supplies);
		require(hall_section(hall_belt).find("[Missing now] " + hall_belt.steps[0].text) !=
					std::string::npos &&
				hall_section(hall_belt).find("[Missing now] " +
							     hall_belt.steps[4].text) !=
					std::string::npos &&
				hall_section(hall_armor)
						.find("[Missing now] " +
						      hall_armor.steps[1].text) !=
					std::string::npos &&
				hall_section(hall_child)
						.find("[Missing now] " +
						      hall_child.steps[0].text) !=
					std::string::npos,
			"Hall short quantities, ordinary steel, local daggers or worn foreign proof fit a recipe");
		supplies.carried[77712] = 2;
		supplies.carried[77748] = 6;
		supplies.carried[18309] = 1;
		supplies.carried[77743] = 1;
		for (const auto item : { 77745, 77713, 77724 })
			supplies.carried[item] = 2;
		for (const auto item : { 77720, 77750 })
			supplies.carried[item] = 1;
		journal = supplied_hall.render_journal(7, 42, 777, 10, 1, 103, false, false,
						       &supplies);
		require(hall_section(hall_belt).find("[Ready now] " + hall_belt.steps[4].text) !=
					std::string::npos &&
				hall_section(hall_belt).find("fourteen-item offering limit") !=
					std::string::npos &&
				hall_section(hall_armor)
						.find("[Ready now] " + hall_armor.steps[6].text) !=
					std::string::npos &&
				hall_section(hall_armor)
						.find("[Pending] " + hall_armor.steps[0].text) !=
					std::string::npos &&
				hall_section(hall_letter)
						.find("[Ready now] " + hall_letter.steps[1].text) !=
					std::string::npos &&
				hall_section(hall_letter)
						.find("[Pending] " + hall_letter.steps[0].text) !=
					std::string::npos &&
				supplied_hall.serialize_state() == hall_before &&
				supplied_hall.progress_for_zone(7, 42, 777).completed == 0,
			"Hall supplies bypassed a recipe limit, manufactured history or changed progress");
		fee_warnings = 0;
		for (size_t at = journal.find(unavailable); at != std::string::npos;
		     at = journal.find(unavailable, at + unavailable.size()))
			++fee_warnings;
		require(fee_warnings == 2 &&
				hall_section(story_for("hall", "shopkeeper-broken-tower-key"))
						.find("separate guarded access service") !=
					std::string::npos,
			"Hall paid key services lost their guarded visibility");
		for (const auto &entry : hall_map.stories)
			if (entry.category == "service")
				record(supplied_hall, entry.contracts.front(), entry.id.c_str(),
				       777, 77908);
		for (const auto &exclusion : hall_map.exclusions)
			record(supplied_hall, exclusion.first, exclusion.first.c_str(), 777, 77906);
		require(supplied_hall.progress_for_zone(7, 42, 777).completed == 0 &&
				supplied_hall.progress_for_zone(7, 42, 777).total == 5,
			"Hall support or elder refusal/shadowed receipts earned achievements");
		record(supplied_hall, hall_letter.contracts.front(), "hall-supplied-letter", 777,
		       77916);
		require(supplied_hall.progress_for_zone(7, 42, 777).completed == 1,
			"Hall supplied letter required personal dagger recovery or the child receipt");
		service recovered_hall(catalog);
		require(recovered_hall.deserialize_state(supplied_hall.serialize_state(), &error),
			"Hall service/exclusion and supplied-letter receipt recovery failed");
		// Historical receipts only: this does not execute the oversized belt or paid services.
		for (const auto &entry : hall_map.stories)
			if (entry.category != "service" && entry.id != hall_letter.id)
				record(recovered_hall, entry.contracts.front(), entry.id.c_str(),
				       777, 77908);
		service restored_hall(catalog);
		require(restored_hall.deserialize_state(recovered_hall.serialize_state(), &error) &&
				restored_hall.progress_for_zone(7, 42, 777).completed == 5 &&
				restored_hall.progress_for_zone(7, 42, 777).total == 5 &&
				restored_hall.progress_for_zone(7, 42, 183).completed == 0,
			"Hall historical recovery merged outcomes, counted exclusions or credited foreign dagger ownership");

		const auto &sarmiz_map = *std::find_if(catalog.story_mappings.begin(),
						       catalog.story_mappings.end(),
						       [](const auto &mapping)
						       { return mapping.source_area == "sarmiz"; });
		const auto &sarmiz_royal = story_for("sarmiz", "rodev-three-ingredients");
		const auto &sarmiz_advisor = story_for("sarmiz", "aberla-four-materials");
		service supplied_sarmiz(catalog);
		require(supplied_sarmiz.discover_zone(7, 42, 94, 9400, 100, "arrival") ==
					result::applied &&
				supplied_sarmiz.render_journal(7, 42, 94, 10, 1, 101, false, false)
						.find("] " + sarmiz_advisor.title + "\r\n") ==
					std::string::npos,
			"Sarmiz discovery revealed an unmet advisor commission");
		for (const auto &contact : sarmiz_map.contacts)
			require(supplied_sarmiz.meet_npc(7, 42, contact.mob_vnum, 9400, 101) ==
					result::applied,
				"Sarmiz fixture encounter failed");
		const auto sarmiz_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Sarmiz journal section missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		for (int item : { 9445, 97135, 97099, 9450, 9442, 9453, 9451 })
			supplies.carried[item] = 1;
		const auto sarmiz_before = supplied_sarmiz.serialize_state();
		journal = supplied_sarmiz.render_journal(7, 42, 94, 10, 1, 102, false, false,
							 &supplies);
		require(sarmiz_section(sarmiz_royal)
						.find("Next: " + sarmiz_royal.steps.back().text) !=
					std::string::npos &&
				sarmiz_section(sarmiz_advisor)
						.find("Next: " +
						      sarmiz_advisor.steps.back().text) !=
					std::string::npos &&
				sarmiz_section(sarmiz_advisor)
						.find("[Pending] " +
						      sarmiz_advisor.steps[0].text) !=
					std::string::npos &&
				supplied_sarmiz.serialize_state() == sarmiz_before &&
				supplied_sarmiz.progress_for_zone(7, 42, 94).completed == 0,
			"supplied Sarmiz recipes required earlier history or a view wrote credit");
		for (const auto *recipe : { &sarmiz_royal, &sarmiz_advisor })
			for (const auto &step : recipe->steps)
			{
				if (step.kind != "carried_item")
					continue;
				const auto item = step.item_vnums.front();
				supplies.carried.erase(item);
				supplies.equipped[16] = item;
				supplies.carried[9454] = 5;
				journal = supplied_sarmiz.render_journal(7, 42, 94, 10, 1, 103,
									 false, false, &supplies);
				require(sarmiz_section(*recipe).find(
						"[Missing now] " + step.text) != std::string::npos,
					"worn ingredient or real blue flasks replaced an exact Sarmiz material");
				supplies.equipped.clear();
				supplies.carried[item] = 1;
			}
		record(supplied_sarmiz, sarmiz_royal.contracts.front(), "sarmiz-supplied-royal", 94,
		       9962);
		require(supplied_sarmiz.progress_for_zone(7, 42, 94).completed == 1 &&
				supplied_sarmiz.progress_for_zone(7, 42, 94).total == 8,
			"royal receipt completed the conspiracy or custom moonstone story");
		record(supplied_sarmiz, sarmiz_advisor.contracts.front(), "sarmiz-supplied-advisor",
		       94, 9962);
		require(supplied_sarmiz.progress_for_zone(7, 42, 94).completed == 2,
			"independent advisor receipt imposed an unrecorded branch or producer history");
		service recovered_sarmiz(catalog);
		require(recovered_sarmiz.deserialize_state(supplied_sarmiz.serialize_state(),
							   &error) &&
				recovered_sarmiz.progress_for_zone(7, 42, 94).completed == 2,
			"Sarmiz cold recovery lost the independent multi-item deliveries");
		for (const auto &entry : sarmiz_map.stories)
			if (entry.id != sarmiz_royal.id && entry.id != sarmiz_advisor.id)
				record(recovered_sarmiz, entry.contracts.front(), entry.id.c_str(),
				       94, 9962);
		service restored_sarmiz(catalog);
		require(restored_sarmiz.deserialize_state(recovered_sarmiz.serialize_state(),
							  &error) &&
				restored_sarmiz.progress_for_zone(7, 42, 94).completed == 8 &&
				restored_sarmiz.progress_for_zone(7, 42, 94).total == 8,
			"Sarmiz recovery merged independent deliveries or invented a custom finale");

		const auto &delwyn_map = *std::find_if(catalog.story_mappings.begin(),
						       catalog.story_mappings.end(),
						       [](const auto &mapping)
						       { return mapping.source_area == "delwyn"; });
		const auto &delwyn_banner = story_for("delwyn", "magician-crimson-banner");
		const auto &delwyn_warning = story_for("delwyn", "duke-note-and-braid");
		service supplied_delwyn(catalog);
		require(supplied_delwyn.discover_zone(7, 42, 828, 82805, 100, "arrival") ==
					result::applied &&
				supplied_delwyn.render_journal(7, 42, 828, 10, 1, 101, false, false)
						.find("] " + delwyn_banner.title + "\r\n") ==
					std::string::npos,
			"Delwyn discovery revealed an unmet magician delivery");
		for (const auto &contact : delwyn_map.contacts)
			require(supplied_delwyn.meet_npc(7, 42, contact.mob_vnum, 82805, 101) ==
					result::applied,
				"Delwyn fixture encounter failed");
		const auto delwyn_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Delwyn journal section missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		for (int item : { 82820, 82824, 82823, 82829, 82818 })
			supplies.carried[item] = 1;
		const auto delwyn_before = supplied_delwyn.serialize_state();
		journal = supplied_delwyn.render_journal(7, 42, 828, 10, 1, 102, false, false,
							 &supplies);
		for (const auto *recipe : { &delwyn_banner, &delwyn_warning })
			require(delwyn_section(*recipe).find("Next: " +
							     recipe->steps.back().text) !=
						std::string::npos &&
					delwyn_section(*recipe).find("[Pending] " +
								     recipe->steps.front().text) !=
						std::string::npos,
				"supplied Delwyn proof required earlier personal producer receipts");
		require(supplied_delwyn.serialize_state() == delwyn_before &&
				supplied_delwyn.progress_for_zone(7, 42, 828).completed == 0 &&
				supplied_delwyn.progress_for_zone(7, 42, 828).total == 6 &&
				delwyn_section(story_for("delwyn", "weaver-crimson-fabric"))
						.find("10000 copper") != std::string::npos,
			"Delwyn material rendering wrote credit, counted fees or lost a paid-service hint");
		for (const auto *recipe : { &delwyn_banner, &delwyn_warning })
			for (const auto &step : recipe->steps)
			{
				if (step.kind != "carried_item")
					continue;
				const auto item = step.item_vnums.front();
				supplies.carried.erase(item);
				supplies.equipped[14] = item;
				supplies.carried[82822] = 5;
				journal = supplied_delwyn.render_journal(7, 42, 828, 10, 1, 103,
									 false, false, &supplies);
				require(delwyn_section(*recipe).find(
						"[Missing now] " + step.text) != std::string::npos,
					"worn or wrong Delwyn evidence replaced an exact root-carried material");
				supplies.equipped.clear();
				supplies.carried[item] = 1;
			}
		const auto &delwyn_weaving = story_for("delwyn", "weaver-crimson-fabric");
		record(supplied_delwyn, delwyn_weaving.contracts.front(), delwyn_weaving.id.c_str(),
		       828, 82872);
		supplies.carried.erase(82819);
		journal = supplied_delwyn.render_journal(7, 42, 828, 10, 1, 104, false, false,
							 &supplies);
		const auto &delwyn_sewing = story_for("delwyn", "seamstress-crimson-banner");
		require(delwyn_section(delwyn_sewing)
						.find("[Recorded] " +
						      delwyn_sewing.steps[0].text) !=
					std::string::npos &&
				delwyn_section(delwyn_sewing)
						.find("[Missing now] " +
						      delwyn_sewing.steps[1].text) !=
					std::string::npos,
			"Delwyn service history replaced spent fabric");
		for (const auto &entry : delwyn_map.stories)
			if (entry.category == "service" && entry.id != delwyn_weaving.id)
				record(supplied_delwyn, entry.contracts.front(), entry.id.c_str(),
				       828, 82872);
		require(supplied_delwyn.progress_for_zone(7, 42, 828).completed == 0,
			"Delwyn paid service history awarded achievement credit");
		record(supplied_delwyn, delwyn_warning.contracts.front(), "delwyn-supplied-warning",
		       828, 82975);
		record(supplied_delwyn, delwyn_banner.contracts.front(), "delwyn-supplied-banner",
		       828, 83006);
		require(supplied_delwyn.progress_for_zone(7, 42, 828).completed == 2,
			"Delwyn warning or banner completed earlier producers or a campaign");
		service recovered_delwyn(catalog);
		require(recovered_delwyn.deserialize_state(supplied_delwyn.serialize_state(),
							   &error) &&
				recovered_delwyn.progress_for_zone(7, 42, 828).completed == 2,
			"Delwyn cold recovery lost separate supplied deliveries");
		for (const auto &entry : delwyn_map.stories)
			if (entry.category != "service" && entry.id != delwyn_warning.id &&
			    entry.id != delwyn_banner.id)
				record(recovered_delwyn, entry.contracts.front(), entry.id.c_str(),
				       828, 82805);
		service restored_delwyn(catalog);
		require(restored_delwyn.deserialize_state(recovered_delwyn.serialize_state(),
							  &error) &&
				restored_delwyn.progress_for_zone(7, 42, 828).completed == 6 &&
				restored_delwyn.progress_for_zone(7, 42, 828).total == 6,
			"Delwyn recovery counted services or invented an all-stage finale");

		const auto &divine_map = *std::find_if(
			catalog.story_mappings.begin(), catalog.story_mappings.end(),
			[](const auto &mapping) { return mapping.source_area == "divhome"; });
		const auto &divine_siren = story_for("divhome", "siren-four-elements");
		const auto &divine_merge = story_for("divhome", "emition-dusk-and-dawn");
		service supplied_divine(catalog);
		require(supplied_divine.discover_zone(7, 42, 407, 40700, 100, "arrival") ==
					result::applied &&
				supplied_divine.render_journal(7, 42, 407, 10, 1, 101, false, false)
						.find("] " + divine_siren.title + "\r\n") ==
					std::string::npos,
			"Divine discovery revealed an unmet siren request");
		for (const auto &contact : divine_map.contacts)
			require(supplied_divine.meet_npc(7, 42, contact.mob_vnum, 40700, 101) ==
					result::applied,
				"Divine fixture encounter failed");
		const auto divine_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Divine journal section missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		for (int item : { 40762, 40763, 40764, 40765, 40780, 40782, 392, 40718, 22032 })
			supplies.carried[item] = 1;
		const auto divine_before = supplied_divine.serialize_state();
		journal = supplied_divine.render_journal(7, 42, 407, 10, 1, 102, false, false,
							 &supplies);
		require(divine_section(divine_siren)
						.find("Next: " + divine_siren.steps.back().text) !=
					std::string::npos &&
				divine_section(divine_merge)
						.find("[Pending] " +
						      divine_merge.steps.front().text) !=
					std::string::npos &&
				divine_section(divine_merge)
						.find("Next: " + divine_merge.steps.back().text) !=
					std::string::npos &&
				divine_section(divine_merge).find("200000 copper") !=
					std::string::npos &&
				divine_section(story_for("divhome", "bounty-relazier"))
						.find("absent") != std::string::npos &&
				supplied_divine.serialize_state() == divine_before &&
				supplied_divine.progress_for_zone(7, 42, 407).total == 20 &&
				supplied_divine.progress_for_zone(7, 42, 407).completed == 0,
			"Divine rendering wrote credit, lost guarded/missing-reward guidance or counted services");
		for (const auto *recipe : { &divine_siren, &divine_merge })
			for (const auto &step : recipe->steps)
			{
				if (step.kind != "carried_item")
					continue;
				const auto item = step.item_vnums.front();
				supplies.carried.erase(item);
				supplies.equipped[14] = item;
				supplies.carried[40770] = 5;
				journal = supplied_divine.render_journal(7, 42, 407, 10, 1, 103,
									 false, false, &supplies);
				require(divine_section(*recipe).find(
						"[Missing now] " + step.text) != std::string::npos,
					"worn or wrong Divine proof replaced an exact carried material");
				supplies.equipped.clear();
				supplies.carried[item] = 1;
			}
		for (const auto &pair : { std::make_pair("wicks-harpy-wyvern-egg", 31105),
					  std::make_pair("wicks-exotic-wyvern-egg", 500026),
					  std::make_pair("wicks-single-horseshoe", 40734),
					  std::make_pair("wicks-moria-horseshoes", 99061) })
		{
			const auto &request = story_for("divhome", pair.first);
			supplies.carried.erase(pair.second);
			supplies.carried[pair.second == 31105  ? 500026 :
					 pair.second == 500026 ? 31105 :
					 pair.second == 40734  ? 99061 :
								 40734] = 4;
			journal = supplied_divine.render_journal(7, 42, 407, 10, 1, 104, false,
								 false, &supplies);
			require(divine_section(request).find("[Missing now] " +
							     request.steps.front().text) !=
					std::string::npos,
				"Divine same-named egg or single/full horseshoes became substitutes");
		}
		const auto &divine_fire_sword = story_for("divhome", "emition-sun-longsword");
		record(supplied_divine, divine_fire_sword.contracts.front(), "divine-fire-service",
		       407, 40757);
		supplies.carried.erase(40780);
		journal = supplied_divine.render_journal(7, 42, 407, 10, 1, 105, false, false,
							 &supplies);
		require(divine_section(divine_merge)
						.find("[Recorded] " +
						      divine_merge.steps.front().text) !=
					std::string::npos &&
				divine_section(divine_merge)
						.find("[Missing now] " +
						      divine_merge.steps[2].text) !=
					std::string::npos,
			"Divine producer receipt replaced its spent fire longsword");
		for (const auto &entry : divine_map.stories)
			if (entry.category == "service" && entry.id != divine_fire_sword.id)
				record(supplied_divine, entry.contracts.front(), entry.id.c_str(),
				       407, 40757);
		require(supplied_divine.progress_for_zone(7, 42, 407).completed == 0,
			"Divine crafting/access service receipts awarded campaign or achievement credit");
		for (const auto *id : { "leolan-titan-token", "raith-titan-token",
					"relazier-puredark", "wicks-exotic-wyvern-egg" })
		{
			const auto &entry = story_for("divhome", id);
			record(supplied_divine, entry.contracts.front(), id, 407, 40717);
		}
		require(supplied_divine.progress_for_zone(7, 42, 407).completed == 4,
			"Divine independent supplied token/foreign/treasure receipts became an exclusive campaign");
		service recovered_divine(catalog);
		require(recovered_divine.deserialize_state(supplied_divine.serialize_state(),
							   &error) &&
				recovered_divine.progress_for_zone(7, 42, 407).completed == 4,
			"Divine cold recovery lost independent receipts");
		for (const auto &entry : divine_map.stories)
			if (entry.category != "service" && entry.id != "leolan-titan-token" &&
			    entry.id != "raith-titan-token" && entry.id != "relazier-puredark" &&
			    entry.id != "wicks-exotic-wyvern-egg")
				record(recovered_divine, entry.contracts.front(), entry.id.c_str(),
				       407, 40717);
		service restored_divine(catalog);
		require(restored_divine.deserialize_state(recovered_divine.serialize_state(),
							  &error) &&
				restored_divine.progress_for_zone(7, 42, 407).completed == 20 &&
				restored_divine.progress_for_zone(7, 42, 407).total == 20,
			"Divine recovery counted support services or invented a full crafting/war finale");

		const auto &halfcut_map = *std::find_if(
			catalog.story_mappings.begin(), catalog.story_mappings.end(),
			[](const auto &mapping) { return mapping.source_area == "halfcut"; });
		const auto &halfcut_badges = story_for("halfcut", "bartis-three-badges");
		const auto &halfcut_note = story_for("halfcut", "sentry-bartis-note");
		const auto &halfcut_scalps = story_for("halfcut", "raid-leader-six-scalps");
		service supplied_halfcut(catalog);
		require(supplied_halfcut.discover_zone(7, 42, 270, 27001, 100, "arrival") ==
					result::applied &&
				supplied_halfcut.render_journal(7, 42, 270, 10, 1, 101, false,
								false)
						.find("] " + halfcut_badges.title + "\r\n") ==
					std::string::npos,
			"Halfcut discovery revealed an unmet Bartis request");
		for (const auto &contact : halfcut_map.contacts)
			require(supplied_halfcut.meet_npc(7, 42, contact.mob_vnum, 27001, 101) ==
					result::applied,
				"Halfcut fixture encounter failed");
		const auto halfcut_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Halfcut journal section missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		for (int item : { 27037, 27039, 27040, 27041, 27042, 27044, 27052, 27053, 27054,
				  27055, 27057, 27058 })
			supplies.carried[item] = 1;
		const auto halfcut_before = supplied_halfcut.serialize_state();
		journal = supplied_halfcut.render_journal(7, 42, 270, 10, 1, 102, false, false,
							  &supplies);
		for (const auto *entry : { &halfcut_badges, &halfcut_note, &halfcut_scalps })
			require(halfcut_section(*entry).find("Next: " + entry->steps.back().text) !=
					std::string::npos,
				"Halfcut supplied proof was blocked by optional personal preparation");
		require(halfcut_section(halfcut_badges)
						.find("[Pending] " +
						      halfcut_badges.steps.front().text) !=
					std::string::npos &&
				halfcut_section(story_for("halfcut", "drow-duergar-scalp"))
						.find("absent") != std::string::npos &&
				supplied_halfcut.serialize_state() == halfcut_before &&
				supplied_halfcut.progress_for_zone(7, 42, 270).total == 13 &&
				supplied_halfcut.progress_for_zone(7, 42, 270).completed == 0,
			"Halfcut hints granted history, hid missing reward or changed independent credit");
		for (const auto *entry : { &halfcut_badges, &halfcut_scalps })
			for (const auto &step : entry->steps)
			{
				if (step.kind != "carried_item")
					continue;
				const auto item = step.item_vnums.front();
				supplies.carried.erase(item);
				supplies.equipped[14] = item;
				supplies.carried[27043] = 6;
				journal = supplied_halfcut.render_journal(7, 42, 270, 10, 1, 103,
									  false, false, &supplies);
				require(halfcut_section(*entry).find(
						"[Missing now] " + step.text) != std::string::npos,
					"Halfcut worn or wrong badge/scalp replaced an exact loose material");
				supplies.equipped.clear();
				supplies.carried[item] = 1;
			}
		const auto &halfcut_medicine = story_for("halfcut", "wounded-dwarf-potion");
		supplies.carried.erase(27037);
		supplies.carried[27046] = 3;
		journal = supplied_halfcut.render_journal(7, 42, 270, 10, 1, 104, false, false,
							  &supplies);
		require(halfcut_section(halfcut_medicine)
					.find("[Missing now] " +
					      halfcut_medicine.steps.front().text) !=
				std::string::npos,
			"Halfcut shop's flaming green potion substituted for wagon medicine");
		for (const auto *id :
		     { "first-old-miner-jar", "second-old-miner-jar", "young-miner-jar" })
		{
			const auto &entry = story_for("halfcut", id);
			record(supplied_halfcut, entry.contracts.front(), id, 270, 27429);
		}
		supplies.carried.erase(27040);
		journal = supplied_halfcut.render_journal(7, 42, 270, 10, 1, 105, false, false,
							  &supplies);
		require(halfcut_section(halfcut_badges)
						.find("[Recorded] " +
						      halfcut_badges.steps.front().text) !=
					std::string::npos &&
				halfcut_section(halfcut_badges)
						.find("[Missing now] " +
						      halfcut_badges.steps[3].text) !=
					std::string::npos &&
				supplied_halfcut.progress_for_zone(7, 42, 270).completed == 3,
			"Halfcut rescue history replaced spent badge or credited the whole campaign");
		const auto &halfcut_final_jar = story_for("halfcut", "bartis-final-jar");
		record(supplied_halfcut, halfcut_final_jar.contracts.front(), "halfcut-final-jar",
		       270, 27433);
		supplies.carried.erase(27044);
		journal = supplied_halfcut.render_journal(7, 42, 270, 10, 1, 106, false, false,
							  &supplies);
		require(halfcut_section(halfcut_note)
						.find("[Recorded] " +
						      halfcut_note.steps.front().text) !=
					std::string::npos &&
				halfcut_section(halfcut_note)
						.find("[Missing now] " +
						      halfcut_note.steps[1].text) !=
					std::string::npos &&
				supplied_halfcut.progress_for_zone(7, 42, 270).completed == 4,
			"Halfcut final jar invented badge bundle, restored recipient or supplied spent note");
		service independent_halfcut(catalog);
		record(independent_halfcut, halfcut_badges.contracts.front(),
		       "halfcut-supplied-badges", 270, 27433);
		record(independent_halfcut, halfcut_note.contracts.front(), "halfcut-supplied-note",
		       270, 27005);
		service restored_halfcut(catalog);
		require(restored_halfcut.deserialize_state(independent_halfcut.serialize_state(),
							   &error) &&
				restored_halfcut.progress_for_zone(7, 42, 270).completed == 2,
			"Halfcut supplied independent bundle/note recovery invented personal rescues");
		for (const auto &entry : halfcut_map.stories)
			if (entry.id != halfcut_badges.id && entry.id != halfcut_note.id)
				record(restored_halfcut, entry.contracts.front(), entry.id.c_str(),
				       270, 27433);
		service recovered_halfcut(catalog);
		require(recovered_halfcut.deserialize_state(restored_halfcut.serialize_state(),
							    &error) &&
				recovered_halfcut.progress_for_zone(7, 42, 270).completed == 13 &&
				recovered_halfcut.progress_for_zone(7, 42, 270).total == 13,
			"Halfcut frozen receipt recovery changed independent identity or invented an extra campaign");

		const auto &scorch_map = *std::find_if(
			catalog.story_mappings.begin(), catalog.story_mappings.end(),
			[](const auto &mapping) { return mapping.source_area == "scorchvalley"; });
		const auto &scorch_blood = story_for("scorchvalley", "seeker-tallin-blood");
		const auto &scorch_rings = story_for("scorchvalley", "wildmage-four-rings");
		const auto &scorch_heads = story_for("scorchvalley", "advisor-three-heads");
		service supplied_scorch(catalog);
		require(supplied_scorch.discover_zone(7, 42, 712, 71201, 100, "arrival") ==
					result::applied &&
				supplied_scorch.render_journal(7, 42, 712, 10, 1, 101, false, false)
						.find("] " + scorch_rings.title + "\r\n") ==
					std::string::npos,
			"Scorched discovery revealed an unmet wildmage request");
		require(supplied_scorch.meet_npc(7, 42, 71253, 29071, 101) == result::rejected &&
				supplied_scorch.meet_npc(7, 42, 71256, 71140, 101) ==
					result::rejected &&
				supplied_scorch.discover_zone(7, 42, 289, 29071, 100, "arrival") ==
					result::applied &&
				supplied_scorch.discover_zone(7, 42, 710, 71140, 100, "arrival") ==
					result::applied,
			"Scorched foreign encounter bypassed physical discovery or valid arrival failed");
		for (const auto &contact : scorch_map.contacts)
		{
			const int room = contact.mob_vnum == 71253 ? 29071 :
					 contact.mob_vnum == 71256 ? 71140 :
								     71201;
			require(supplied_scorch.meet_npc(7, 42, contact.mob_vnum, room, 101) ==
					result::applied,
				"Scorched fixture foreign/local encounter failed");
		}
		const auto scorch_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Scorched journal section missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		for (int item : { 71224, 71244, 71245, 71246, 71239, 71227, 71009, 28980 })
			supplies.carried[item] = 1;
		const auto scorch_before = supplied_scorch.serialize_state();
		journal = supplied_scorch.render_journal(7, 42, 712, 10, 1, 102, false, false,
							 &supplies);
		for (const auto *entry : { &scorch_blood, &scorch_rings, &scorch_heads })
			require(scorch_section(*entry).find("Next: " + entry->steps.back().text) !=
					std::string::npos,
				"Scorched supplied proof was blocked by personal keys, history or foreign campaign");
		require(scorch_section(scorch_rings)
						.find("[Pending] " +
						      scorch_rings.steps.front().text) !=
					std::string::npos &&
				scorch_section(scorch_blood)
						.find("[Missing now] " +
						      scorch_blood.steps.front().text) !=
					std::string::npos &&
				supplied_scorch.serialize_state() == scorch_before &&
				supplied_scorch.progress_for_zone(7, 42, 712).total == 9 &&
				supplied_scorch.progress_for_zone(7, 42, 712).completed == 0,
			"Scorched optional route checks granted access, history or credit");
		for (const auto &step : scorch_rings.steps)
		{
			if (step.kind != "carried_item")
				continue;
			const auto item = step.item_vnums.front();
			supplies.carried.erase(item);
			supplies.equipped[14] = item;
			supplies.carried[71205] = 4;
			journal = supplied_scorch.render_journal(7, 42, 712, 10, 1, 103, false,
								 false, &supplies);
			require(scorch_section(scorch_rings).find("[Missing now] " + step.text) !=
					std::string::npos,
				"Scorched worn or heirloom proof replaced an exact colored ring");
			supplies.equipped.clear();
			supplies.carried[item] = 1;
		}
		supplies.carried.erase(71227);
		supplies.carried[71240] = 1;
		journal = supplied_scorch.render_journal(7, 42, 712, 10, 1, 104, false, false,
							 &supplies);
		require(scorch_section(scorch_heads)
					.find("[Missing now] " + scorch_heads.steps.front().text) !=
				std::string::npos,
			"Scorched other commander's magic substituted for the accepted head");
		record(supplied_scorch, scorch_blood.contracts.front(), "scorch-blood", 712, 71140);
		supplies.carried.erase(71224);
		journal = supplied_scorch.render_journal(7, 42, 712, 10, 1, 105, false, false,
							 &supplies);
		require(scorch_section(scorch_rings)
						.find("[Recorded] " +
						      scorch_rings.steps.front().text) !=
					std::string::npos &&
				scorch_section(scorch_rings)
						.find("[Missing now] " +
						      scorch_rings.steps[1].text) !=
					std::string::npos &&
				supplied_scorch.progress_for_zone(7, 42, 712).completed == 1 &&
				supplied_scorch.progress_for_zone(7, 42, 710).completed == 0,
			"Scorched producer history restored spent rings, completed necklace or transferred ownership");
		service independent_scorch(catalog);
		record(independent_scorch, scorch_rings.contracts.front(), "scorch-supplied-rings",
		       712, 71264);
		record(independent_scorch,
		       story_for("scorchvalley", "council-godly-magic").contracts.front(),
		       "scorch-council", 712, 29071);
		service restored_scorch(catalog);
		require(restored_scorch.deserialize_state(independent_scorch.serialize_state(),
							  &error) &&
				restored_scorch.progress_for_zone(7, 42, 712).completed == 2 &&
				restored_scorch.progress_for_zone(7, 42, 289).completed == 0,
			"Scorched supplied necklace/council recovery invented producer or foreign credit");
		for (const auto &entry : scorch_map.stories)
			if (entry.id != scorch_rings.id &&
			    entry.id != story_for("scorchvalley", "council-godly-magic").id)
				record(restored_scorch, entry.contracts.front(), entry.id.c_str(),
				       712, 71283);
		service recovered_scorch(catalog);
		require(recovered_scorch.deserialize_state(restored_scorch.serialize_state(),
							   &error) &&
				recovered_scorch.progress_for_zone(7, 42, 712).completed == 9 &&
				recovered_scorch.progress_for_zone(7, 42, 712).total == 9,
			"Scorched frozen recovery changed independent identity or invented an extra campaign");

		const auto &court_map = *std::find_if(catalog.story_mappings.begin(),
						      catalog.story_mappings.end(),
						      [](const auto &mapping)
						      { return mapping.source_area == "court"; });
		const auto &court_admission = story_for("court", "priestess-four-seasons");
		const auto &court_fisherman = story_for("court", "fisherman-dozen-scales");
		const auto &court_winter = story_for("court", "winter-snowflake");
		service supplied_court(catalog);
		require(supplied_court.discover_zone(7, 42, 67, 6716, 100, "arrival") ==
					result::applied &&
				supplied_court.render_journal(7, 42, 67, 10, 1, 101, false, false)
						.find("] " + court_admission.title + "\r\n") ==
					std::string::npos,
			"Court discovery revealed an unmet admission request");
		for (const auto &contact : court_map.contacts)
			require(supplied_court.meet_npc(7, 42, contact.mob_vnum, 6704, 101) ==
					result::applied,
				"Court synthetic encounter fixture failed");
		const auto court_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Court journal section missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		for (int item : { 6714, 6715, 6716, 6717 })
			supplies.carried[item] = 1;
		supplies.carried[6702] = 11;
		supplies.carried[6725] = 1;
		const auto court_before = supplied_court.serialize_state();
		journal = supplied_court.render_journal(7, 42, 67, 10, 1, 102, false, false,
							&supplies);
		require(court_section(court_admission)
						.find("Next: " +
						      court_admission.steps.back().text) !=
					std::string::npos &&
				court_section(court_admission)
						.find("[Pending] " +
						      court_admission.steps.front().text) !=
					std::string::npos &&
				court_section(court_fisherman)
						.find("[Missing now] " +
						      court_fisherman.steps.front().text) !=
					std::string::npos &&
				court_section(court_winter)
						.find("[Missing now] " +
						      court_winter.steps.front().text) !=
					std::string::npos,
			"Court supplied seasons required personal favors, or eleven scales/worn-kind snowflake fit");
		supplies.carried[6702] = 12;
		journal = supplied_court.render_journal(7, 42, 67, 10, 1, 103, false, false,
							&supplies);
		require(court_section(court_fisherman)
						.find("[Ready now] " +
						      court_fisherman.steps.front().text) !=
					std::string::npos &&
				supplied_court.serialize_state() == court_before &&
				supplied_court.progress_for_zone(7, 42, 67).total == 9 &&
				supplied_court.progress_for_zone(7, 42, 67).completed == 0,
			"Court twelve-copy readiness mutated state or granted a catch, ritual or completion");
		for (const auto &step : court_admission.steps)
		{
			if (step.kind != "carried_item")
				continue;
			const auto item = step.item_vnums.front();
			supplies.carried.erase(item);
			supplies.equipped[14] = item;
			supplies.carried[6713] = 1;
			journal = supplied_court.render_journal(7, 42, 67, 10, 1, 104, false, false,
								&supplies);
			require(court_section(court_admission).find("[Missing now] " + step.text) !=
					std::string::npos,
				"Court worn token or supplied key replaced an exact seasonal token");
			supplies.equipped.clear();
			supplies.carried[item] = 1;
		}
		supplies.carried.erase(6715);
		supplies.carried[6714] = 4;
		journal = supplied_court.render_journal(7, 42, 67, 10, 1, 105, false, false,
							&supplies);
		require(court_section(court_admission)
					.find("[Missing now] " + court_admission.steps[5].text) !=
				std::string::npos,
			"Court repeated Spring token replaced distinct Autumn proof");
		supplies.carried[6715] = 1;
		supplies.carried.erase(6717);
		record(supplied_court, court_winter.contracts.front(), "court-winter", 67, 6788);
		journal = supplied_court.render_journal(7, 42, 67, 10, 1, 106, false, false,
							&supplies);
		require(court_section(court_admission)
						.find("[Recorded] " +
						      court_admission.steps[3].text) !=
					std::string::npos &&
				court_section(court_admission)
						.find("[Missing now] " +
						      court_admission.steps[7].text) !=
					std::string::npos &&
				supplied_court.progress_for_zone(7, 42, 67).completed == 1,
			"Court favor history restored spent Winter proof or completed admission");
		service independent_court(catalog);
		record(independent_court, court_admission.contracts.front(),
		       "court-supplied-admission", 67, 6704);
		service restored_court(catalog);
		require(restored_court.deserialize_state(independent_court.serialize_state(),
							 &error) &&
				restored_court.progress_for_zone(7, 42, 67).completed == 1,
			"Court supplied admission recovery invented four personal favors or an audience");
		for (const auto &entry : court_map.stories)
			if (entry.id != court_admission.id)
				record(restored_court, entry.contracts.front(), entry.id.c_str(),
				       67, 6704);
		service recovered_court(catalog);
		require(recovered_court.deserialize_state(restored_court.serialize_state(),
							  &error) &&
				recovered_court.progress_for_zone(7, 42, 67).completed == 9 &&
				recovered_court.progress_for_zone(7, 42, 67).total == 9,
			"Court recovery multiplied friend instances or invented a ritual/campaign finale");

		const auto &snogres_map = *std::find_if(
			catalog.story_mappings.begin(), catalog.story_mappings.end(),
			[](const auto &mapping) { return mapping.source_area == "snogres"; });
		const auto &snogres_pyramid = story_for("snogres", "lich-three-shards");
		const auto &snogres_green = story_for("snogres", "lich-reverse-hourglass");
		const auto &snogres_tentacle = story_for("snogres", "lich-illithid-tentacle");
		const auto &snogres_armor = story_for("snogres", "leppts-remorhaz-armor");
		service supplied_snogres(catalog);
		require(supplied_snogres.discover_zone(7, 42, 877, 87700, 100, "arrival") ==
					result::applied &&
				supplied_snogres.render_journal(7, 42, 877, 10, 1, 101, false,
								false)
						.find("] " + snogres_pyramid.title + "\r\n") ==
					std::string::npos,
			"Snow discovery revealed an unmet lich request");
		require(supplied_snogres.meet_npc(7, 42, 87742, 660001, 101) == result::rejected &&
				supplied_snogres.discover_zone(7, 42, 5000, 660001, 101,
							       "arrival") == result::applied &&
				supplied_snogres.meet_npc(7, 42, 87742, 660001, 102) ==
					result::applied,
			"Snow foreign tinker ignored physical surface discovery");
		require(supplied_snogres.meet_npc(7, 42, 87733, 87798, 102) == result::applied,
			"Snow lich encounter fixture failed");
		const auto snogres_section = [&](const auto &entry)
		{
			const auto start = journal.find("] " + entry.title + "\r\n");
			require(start != std::string::npos, "Snow journal section missing");
			return journal.substr(start, journal.find("\r\n  [", start) - start);
		};
		supplies = {};
		for (int item : { 87713, 87730, 87731, 87710, 87711 })
			supplies.carried[item] = 1;
		supplies.carried[87725] = 5;
		supplies.carried[87724] = 1;
		const auto snogres_before = supplied_snogres.serialize_state();
		journal = supplied_snogres.render_journal(7, 42, 877, 10, 1, 103, false, false,
							  &supplies);
		require(snogres_section(snogres_pyramid)
						.find("Next: " +
						      snogres_pyramid.steps.back().text) !=
					std::string::npos &&
				snogres_section(snogres_pyramid)
						.find("[Pending] " +
						      snogres_pyramid.steps.front().text) !=
					std::string::npos &&
				snogres_section(snogres_armor)
						.find("[Missing now] " +
						      snogres_armor.steps.front().text) !=
					std::string::npos &&
				snogres_section(snogres_armor).find("2,500 platinum") !=
					std::string::npos &&
				snogres_section(snogres_armor).find("guarded") !=
					std::string::npos &&
				snogres_section(snogres_tentacle)
						.find("[Missing now] " +
						      snogres_tentacle.steps.front().text) !=
					std::string::npos,
			"Snow supplied shards required histories, five hides fit, fee vanished or whip replaced tentacle");
		supplies.carried[87725] = 6;
		journal = supplied_snogres.render_journal(7, 42, 877, 10, 1, 104, false, false,
							  &supplies);
		require(snogres_section(snogres_armor)
						.find("[Ready now] " +
						      snogres_armor.steps.front().text) !=
					std::string::npos &&
				supplied_snogres.serialize_state() == snogres_before &&
				supplied_snogres.progress_for_zone(7, 42, 877).total == 7 &&
				supplied_snogres.progress_for_zone(7, 42, 877).completed == 0,
			"Snow readiness wrote state, qualified payment or awarded a personal kill");
		for (const auto &step : snogres_pyramid.steps)
		{
			if (step.kind != "carried_item")
				continue;
			const auto item = step.item_vnums.front();
			supplies.carried.erase(item);
			supplies.equipped[14] = item;
			supplies.carried[87732] = 1;
			journal = supplied_snogres.render_journal(7, 42, 877, 10, 1, 105, false,
								  false, &supplies);
			require(snogres_section(snogres_pyramid).find("[Missing now] " + step.text) !=
					std::string::npos,
				"Snow worn shard or reward pyramid replaced exact proof");
			supplies.equipped.clear();
			supplies.carried[item] = 1;
		}
		supplies.carried.erase(87730);
		supplies.carried[87713] = 3;
		journal = supplied_snogres.render_journal(7, 42, 877, 10, 1, 106, false, false,
							  &supplies);
		require(snogres_section(snogres_pyramid)
					.find("[Missing now] " + snogres_pyramid.steps[4].text) !=
				std::string::npos,
			"Snow duplicate green shards replaced the distinct yellow kind");
		supplies.carried[87730] = 1;
		supplies.carried.erase(87713);
		record(supplied_snogres, snogres_green.contracts.front(), "snogres-green", 877,
		       87798);
		journal = supplied_snogres.render_journal(7, 42, 877, 10, 1, 107, false, false,
							  &supplies);
		require(snogres_section(snogres_pyramid)
						.find("[Recorded] " +
						      snogres_pyramid.steps.front().text) !=
					std::string::npos &&
				snogres_section(snogres_pyramid)
						.find("[Missing now] " +
						      snogres_pyramid.steps[3].text) !=
					std::string::npos,
			"Snow producer history restored spent green proof");
		// Synthetic receipt projection: the guarded mixed coin exchange is not executed here.
		auto wrong_snogres_owner =
			completion(snogres_armor.contracts.front(), "snogres-wrong-owner", 120);
		wrong_snogres_owner.transaction.zone_number = 5000;
		wrong_snogres_owner.transaction.room_vnum = 660001;
		require(supplied_snogres.record_completion(wrong_snogres_owner) == result::rejected,
			"Snow foreign service accepted physical area as contract owner");
		record(supplied_snogres, snogres_armor.contracts.front(), "snogres-armor-service",
		       877, 660001);
		for (const auto &excluded : snogres_map.exclusions)
			record(supplied_snogres, excluded.first, "snogres-hide-refusal", 877,
			       87798);
		require(supplied_snogres.progress_for_zone(7, 42, 877).completed == 1 &&
				supplied_snogres.progress_for_zone(7, 42, 5000).completed == 0,
			"Snow service/refusal awarded achievement or foreign physical zone stole ownership");
		service independent_snogres(catalog);
		record(independent_snogres, snogres_pyramid.contracts.front(),
		       "snogres-supplied-pyramid", 877, 87798);
		service restored_snogres(catalog);
		require(restored_snogres.deserialize_state(independent_snogres.serialize_state(),
							   &error) &&
				restored_snogres.progress_for_zone(7, 42, 877).completed == 1,
			"Snow supplied pyramid recovery invented three producer histories or a ritual");
		for (const auto &entry : snogres_map.stories)
			if (entry.category != "service" && entry.id != snogres_pyramid.id)
				record(restored_snogres, entry.contracts.front(), entry.id.c_str(),
				       877, 87798);
		service recovered_snogres(catalog);
		require(recovered_snogres.deserialize_state(restored_snogres.serialize_state(),
							    &error) &&
				recovered_snogres.progress_for_zone(7, 42, 877).completed == 7 &&
				recovered_snogres.progress_for_zone(7, 42, 877).total == 7,
			"Snow recovery counted service/refusal or invented kills, transformation or reunion");
		std::cout
			<< "All mappings, optional preparation, independent story journeys, exact materials, service exclusion, mixed-fee visibility, and receipt recovery passed.\n";
		return 0;
	}
	const bool applied = zone_story_quest_story::apply(read(argv[2]), "twin_towers_forest",
							   &catalog, &error);
	if (argc > 3 && std::string(argv[3]) == "invalid")
	{
		require(!applied && !error.empty() && catalog.story_mappings.empty() &&
				zone_story_quest_catalog::eligible_definition_count(catalog, 135,
										    2) == 84,
			"invalid mapping mutated the catalog or lacked a diagnostic");
		return 0;
	}
	if (!applied)
		std::cerr << error << '\n';
	require(applied, "production mapping was rejected");
	if (argc > 3)
	{
		const bool service = std::string(argv[3]) == "service";
		require(zone_story_quest_catalog::eligible_definition_count(catalog, 135, 2) ==
				(service ? 76 : 77),
			"partial mapping lost unbound native contracts");
		const auto units = zone_story_quest_catalog::quest_units(catalog);
		require(units.front().achievement == !service &&
				units.front().daily_candidate == !service,
			"service category contributed achievement or daily units");
		return 0;
	}
	require(catalog.definitions.size() == 2668 && catalog.content_revision == 2,
		"mapping rewrote native definitions");
	require(zone_story_quest_catalog::eligible_definition_count(catalog, 135, 2) == 10,
		"Twin Towers stories were not grouped");
	const auto &flowers = catalog.story_mappings.front().stories.front();
	require(flowers.contracts.size() == 8 && flowers.steps.front().slot == 13,
		"flower alternatives or garden gate changed");
	service tracker(catalog);
	const int64_t now = 172800100;
	require(tracker.discover_zone(7, 42, 135, 13556, now - 10, "arrival") == result::applied,
		"discovery failed");
	zone_story_quest_catalog::journal_inventory inventory;
	require(tracker.render_journal(7, 42, 135, 10, 1, now, false, false).find(flowers.title) ==
			std::string::npos,
		"zone discovery exposed an unseen giver's story");
	require(tracker.meet_npc(7, 42, 13500, 13556, now - 9) == result::applied,
		"physical encounter failed");
	inventory.carried[13521] = 1;
	inventory.carried[flowers.steps[1].item_vnums.front()] = 1;
	auto journal = tracker.render_journal(7, 42, 135, 10, 1, now, false, false, &inventory);
	require(journal.find("[Missing now] " + flowers.steps[0].text) != std::string::npos &&
			journal.find("[Ready now] " + flowers.steps[1].text) != std::string::npos,
		"carrying the belt falsely opened the garden");
	if (flowers.steps[0].optional)
		require(journal.find("Optional preparation:") != std::string::npos &&
				journal.find("Next: " + flowers.steps[0].text) ==
					std::string::npos &&
				journal.find("Next: " + flowers.steps.back().text) !=
					std::string::npos,
			"a supplied plant incorrectly required the garden access route");
	inventory.equipped[0] = 13521;
	require(tracker.render_journal(7, 42, 135, 10, 1, now, false, false, &inventory)
				.find("[Missing now] " + flowers.steps[0].text) !=
			std::string::npos,
		"wrong equipment slot opened the garden");
	inventory.equipped[13] = 13521;
	const auto state_before_read = tracker.serialize_state();
	journal = tracker.render_journal(7, 42, 135, 10, 1, now, false, false, &inventory);
	require(journal.find("[Ready now] " + flowers.steps[0].text) != std::string::npos &&
			tracker.serialize_state() == state_before_read,
		"live checks mutated durable history");
	daily_policy policy;
	policy.enabled = true;
	policy.minimum_attempts = policy.minimum_distinct_pids = policy.minimum_successes = 1;
	tracker.set_daily_policy(policy);
	telemetry_observation observation;
	observation.observation_id = "verified-alternate";
	observation.quest_definition_id = flowers.contracts[1];
	observation.content_revision = 2;
	observation.observed_at = now - 1;
	observation.pid = 43;
	observation.level = observation.strongest_party_level = 10;
	observation.racewar = 1;
	observation.credit_mask = zone_story_quest_tracking::ZONE_STORY_CREDIT_PERSONAL;
	observation.party_size = 1;
	observation.outcome = telemetry_outcome::success;
	observation.accessible = true;
	require(tracker.record_telemetry(observation) == result::applied &&
			tracker.daily_eligible_for(7, 42, flowers.contracts[1], 10, 1, 10, now),
		"verified daily alternative was unavailable");
	require(tracker.record_completion(completion(flowers.contracts[0], "flower-first", now)) ==
			result::applied,
		"flower receipt failed");
	require(!tracker.daily_eligible_for(7, 42, flowers.contracts[1], 10, 1, 10, now + 1),
		"alternate earned another daily unit");
	require(tracker.record_completion(completion(flowers.contracts[1], "flower-alternate",
						     now + 1)) == result::applied,
		"alternate receipt was lost");
	const auto rejected = catalog.story_mappings.front().exclusions.begin()->first;
	require(!tracker.daily_eligible_for(7, 42, rejected, 10, 1, 10, now + 2),
		"excluded exchange became daily");
	require(tracker.record_completion(completion(rejected, "reviewed-exclusion", now + 2)) ==
			result::applied,
		"mapping blocked native receipt history");
	const auto progress = tracker.progress_for_zone(7, 42, 135);
	require(progress.completed == 1 && progress.total == 10 &&
			tracker.summary_for(7, 42).total == 2585 &&
			tracker.summary_for(7, 42).renown == 1,
		"grouping double-counted progress or reward");
	journal = tracker.render_journal(7, 42, 135, 10, 1, now + 2, true, false, &inventory);
	require(journal.find("[Done today] " + flowers.title) != std::string::npos,
		"daily journal did not project alternatives");
	const auto persisted = tracker.serialize_state();
	service restored(catalog);
	require(restored.deserialize_state(persisted, &error) &&
			restored.progress_for_zone(7, 42, 135).completed == 1,
		"restart lost authored story progress");
	service legacy(raw_catalog);
	require(legacy.deserialize_state(persisted, &error) &&
			legacy.progress_for_zone(7, 42, 135).completed == 3,
		"mapping removed original receipts");
	const auto board = restored.leaderboard(7, 0, 0, 10, 42, now + 86400);
	require(board.entries.size() == 1 && board.entries.front().completed == 1,
		"leaderboard double-counted terminal alternatives");
	inventory.carried.clear();
	inventory.equipped.clear();
	journal = restored.render_journal(7, 42, 135, 10, 1, now + 2, false, false, &inventory);
	require(journal.find("[Story complete") != std::string::npos &&
			journal.find("[Recorded] " + flowers.steps.back().text) !=
				std::string::npos,
		"consuming materials erased completion");
	service copied = restored;
	restored = service(raw_catalog);
	require(copied.progress_for_zone(7, 42, 135).completed == 1,
		"copied projection retained dangling catalog pointers");
	const auto &glor = catalog.story_mappings.front().stories[2];
	unsigned feathers = 0;
	for (const auto &step : glor.steps)
		if (step.kind == "carried_item" && step.item_vnums.size() == 1 &&
		    step.item_vnums[0] >= 13584 && step.item_vnums[0] <= 13587)
			++feathers;
	require(feathers == 4, "different feathers collapsed into interchangeable items");
	require(!zone_story_quest_story::apply(read(argv[2]), "twin_towers_forest", &catalog,
					       &error) &&
			catalog.story_mappings.size() == 1,
		"duplicate area mapping was accepted");
	std::cout
		<< "authored stories, live equipment, alternative credit, daily projection, and retained receipts passed\n";
}
