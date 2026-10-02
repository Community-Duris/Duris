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
		require(catalog.story_mappings.size() == 38 &&
				tracker.summary_for(7, 42).total == 2357,
			"native story projection disagreed with the complete source audit");
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
