#include "world/zone_story_quest_feature.h"
#include "world/zone_story_quest_story.h"

#include <cjson/cJSON.h>
#include <algorithm>
#include <cstdlib>
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
			const auto &zone = *std::find_if(
				catalog.zones.begin(), catalog.zones.end(), [&](const auto &z)
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
		require(catalog.story_mappings.size() == 36 &&
				tracker.summary_for(7, 42).total == 2359,
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
		std::cout
			<< "All mappings, fish grouping, supplied-note/collar guidance, and receipt recovery passed.\n";
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
