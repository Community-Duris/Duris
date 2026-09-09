#include "world/zone_story_quest_catalog.h"
#include "world/zone_story_quest_tracking.h"

#include <cstdlib>
#include <iostream>
#include <string>

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

zone_story_quest_tracking::quest_definition definition(const char *id, int32_t zone)
{
	return { .definition_id = id,
		 .source_system = "zone_story",
		 .zone_number = zone,
		 .source_area = "fixture-area",
		 .giver_vnum = 1001,
		 .completion_key = id,
		 .active = true,
		 .eligible_for_zone_completion = true,
		 .repeatable = true,
		 .content_revision = 7 };
}
} // namespace

int main()
{
	using namespace zone_story_quest_tracking;

	const quest_definition quest = definition("zone-story:900:001", 900);
	std::string error;
	require(validate_definition(quest, &error), "valid quest definition rejected");

	completion_transaction solo = { .schema_version = ZONE_STORY_QUEST_TRACKING_SCHEMA_VERSION,
					.transaction_id = "tx-solo",
					.quest_definition_id = quest.definition_id,
					.zone_number = 900,
					.direct_completer_pid = 42,
					.credited_pids = { 42 },
					.room_vnum = 90001,
					.completed_at = 1700000000,
					.season_id = 3,
					.content_revision = 7 };
	require(validate_transaction(solo, &error), "valid solo transaction rejected");
	require(is_solo_transaction(solo), "single recipient was not solo");
	require(!is_leadership_transaction(solo), "single recipient was leadership");
	require(credit_mask_for_pid(solo, 42) ==
			(ZONE_STORY_CREDIT_PERSONAL | ZONE_STORY_CREDIT_SOLO),
		"solo credit mask was incorrect");
	require(credit_mask_for_pid(solo, 99) == ZONE_STORY_CREDIT_NONE,
		"non-recipient received credit");

	completion_transaction group = solo;
	group.transaction_id = "tx-group";
	group.credited_pids = { 42, 77, 91 };
	require(validate_transaction(group, &error), "valid group transaction rejected");
	require(!is_solo_transaction(group), "group transaction was solo");
	require(is_leadership_transaction(group), "direct completer was not leadership");
	require(credit_mask_for_pid(group, 42) ==
			(ZONE_STORY_CREDIT_PERSONAL | ZONE_STORY_CREDIT_GROUP_PARTICIPANT |
			 ZONE_STORY_CREDIT_LEADERSHIP),
		"direct completer group credit mask was incorrect");
	require(credit_mask_for_pid(group, 77) == ZONE_STORY_CREDIT_GROUP_PARTICIPANT,
		"group participant credit mask was incorrect");

	completion_transaction duplicate = group;
	duplicate.transaction_id = "tx-duplicate-recipient";
	duplicate.credited_pids = { 42, 77, 77 };
	require(!validate_transaction(duplicate, &error),
		"duplicate credited recipient was accepted");

	completion_transaction missing_direct = group;
	missing_direct.transaction_id = "tx-missing-direct";
	missing_direct.credited_pids = { 77, 91 };
	require(!validate_transaction(missing_direct, &error),
		"transaction without direct completer was accepted");

	const std::string encoded = serialize_transaction(group);
	completion_transaction decoded = {};
	require(deserialize_transaction(encoded, &decoded, &error),
		"serialized transaction did not decode");
	require(serialize_transaction(decoded) == encoded,
		"transaction serialization was not deterministic");

	completion_transaction unsupported = group;
	unsupported.schema_version++;
	require(!validate_transaction(unsupported, &error),
		"unsupported transaction schema was accepted");

	std::string serialize_err;
	require(serialize_transaction(unsupported, &serialize_err).empty(),
		"invalid transaction serialization did not return empty string");
	require(!serialize_err.empty(),
		"invalid transaction serialization did not set error string");

	quest_definition default_quest;
	require(!validate_definition(default_quest, &error),
		"default quest definition should be invalid");
	require(!default_quest.active && !default_quest.repeatable &&
			default_quest.zone_number == 0 && default_quest.giver_vnum == 0,
		"quest definition default values were not initialized");

	completion_transaction default_tx;
	require(!validate_transaction(default_tx, &error),
		"default completion transaction should be invalid");
	require(default_tx.schema_version == 0 && default_tx.direct_completer_pid == 0,
		"completion transaction default values were not initialized");

	zone_story_quest_catalog::catalog catalog = {
		.content_revision = 7,
		.definitions = { definition("zone-story:900:001", 900),
				 definition("zone-story:900:002", 900),
				 definition("zone-story:901:001", 901) }
	};
	std::vector<zone_story_quest_catalog::diagnostic> diagnostics;
	require(zone_story_quest_catalog::validate(catalog, &diagnostics),
		"valid catalog rejected");
	require(zone_story_quest_catalog::eligible_definition_count(catalog, 900, 7) == 2,
		"zone definition count was incorrect");
	require(zone_story_quest_catalog::eligible_definition_count(catalog, 901, 7) == 1,
		"second zone definition count was incorrect");

	zone_story_quest_catalog::catalog non_repeatable_catalog = {
		.content_revision = 7, .definitions = { definition("zone-story:900:001", 900) }
	};
	non_repeatable_catalog.definitions[0].repeatable = false;
	require(zone_story_quest_catalog::eligible_definition_count(non_repeatable_catalog, 900,
								    7) == 1,
		"eligible_definition_count should not filter on repeatable");

	zone_story_quest_catalog::catalog invalid_catalog = { .schema_version = 999,
							      .content_revision = 0,
							      .definitions = {} };
	diagnostics.clear();
	require(!zone_story_quest_catalog::validate(invalid_catalog, &diagnostics),
		"invalid catalog was accepted");
	require(diagnostics.size() == 2, "expected 2 catalog-level diagnostics");
	require(diagnostics[0].index == -1 && diagnostics[1].index == -1,
		"catalog-level diagnostics must have index -1");

	catalog.definitions.push_back(definition("zone-story:900:001", 900));
	diagnostics.clear();
	require(!zone_story_quest_catalog::validate(catalog, &diagnostics),
		"duplicate catalog definition was accepted");
	require(!diagnostics.empty(), "catalog failure had no diagnostic");

	std::cout << "zone-story quest tracking contract regression passed\n";
	return 0;
}
