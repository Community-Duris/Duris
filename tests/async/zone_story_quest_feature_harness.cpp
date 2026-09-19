#include "world/zone_story_quest_feature.h"

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

zone_story_quest_tracking::quest_definition definition(const char *id, int zone)
{
	return {
		.definition_id = id,
		.source_system = "zone_story",
		.zone_number = zone,
		.source_area = "production-qst",
		.giver_vnum = zone * 100 + 1,
		.completion_key = id,
		.active = true,
		.eligible_for_zone_completion = true,
		.repeatable = true,
		.content_revision = 7,
		.display_name = zone == 900 ? "Recover the harbor sigil" : "Carry the dusk message",
		.giver_name = zone == 900 ? "the harbor master" : "the dusk archivist",
		.zone_name = zone == 900 ? "The Ember Coast" : "The Dusk Archive",
		.objective = zone == 900 ? "Bring the lost sigil back to the harbor master." :
					   "Deliver the sealed message to the dusk archivist.",
	};
}

zone_story_quest_feature::completion_event completion(const char *txid, const char *quest, int zone,
						      uint32_t direct, int64_t at,
						      std::vector<uint32_t> credited)
{
	zone_story_quest_feature::completion_event event;
	event.transaction = {
		.schema_version =
			zone_story_quest_tracking::ZONE_STORY_QUEST_TRACKING_SCHEMA_VERSION,
		.transaction_id = txid,
		.quest_definition_id = quest,
		.zone_number = zone,
		.direct_completer_pid = direct,
		.credited_pids = std::move(credited),
		.room_vnum = zone * 100 + 10,
		.completed_at = at,
		.season_id = 7,
		.content_revision = 7
	};
	event.character_name = direct == 42 ? "Alice" : direct == 77 ? "Bob" : "Cara";
	event.level = 10;
	event.racewar = 1;
	event.party_context_known = event.transaction.credited_pids.size() > 1;
	event.party_size = static_cast<uint32_t>(event.transaction.credited_pids.size());
	event.strongest_party_level = 10;
	return event;
}
} // namespace

int main()
{
	using namespace zone_story_quest_feature;
	using namespace zone_story_quest_tracking;

	zone_story_quest_catalog::catalog catalog = {
		.content_revision = 7,
		.definitions = { definition("zone-story:900:001", 900),
				 definition("zone-story:900:002", 900),
				 definition("zone-story:901:001", 901),
				 definition("zone-story:901:002", 901) }
	};
	service tracker(catalog);
	std::string error;
	completion_event conflicting =
		completion("tx-conflict", "zone-story:900:001", 900, 42, 172800050, { 42 });
	telemetry_observation collision;
	collision.observation_id = "completion:tx-conflict";
	collision.quest_definition_id = "zone-story:900:002";
	collision.content_revision = 7;
	collision.observed_at = conflicting.transaction.completed_at;
	collision.pid = 42;
	collision.level = 10;
	collision.racewar = 1;
	collision.party_size = 1;
	collision.strongest_party_level = 10;
	collision.outcome = telemetry_outcome::success;
	collision.accessible = true;
	require(tracker.record_telemetry(collision, &error) == result::applied,
		"telemetry collision fixture was not applied");
	require(tracker.record_completion(conflicting, &error) == result::conflict &&
			tracker.summary_for(7, 42).completed == 0,
		"telemetry conflict left a phantom completion behind");

	completion_event first =
		completion("tx-1", "zone-story:900:001", 900, 42, 172800100, { 42 });
	require(tracker.record_completion(first, &error) == result::applied,
		"first completion was not applied");
	require(tracker.record_completion(first, &error) == result::already_applied,
		"replayed completion was not idempotent");

	completion_event group =
		completion("tx-2", "zone-story:900:002", 900, 42, 172800200, { 42, 77 });
	require(tracker.record_completion(group, &error) == result::applied,
		"group completion was not applied");
	require(credit_mask_for_pid(group.transaction, 42) ==
			(ZONE_STORY_CREDIT_PERSONAL | ZONE_STORY_CREDIT_GROUP_PARTICIPANT |
			 ZONE_STORY_CREDIT_LEADERSHIP),
		"leadership mask was not preserved");

	completion_event third =
		completion("tx-3", "zone-story:901:001", 901, 77, 172800300, { 77 });
	require(tracker.record_completion(third, &error) == result::applied,
		"second character completion was not applied");

	tracker.remember_character(7, 91, "Offline");
	const zone_progress alice_zone = tracker.progress_for_zone(7, 42, 900);
	require(alice_zone.completed == 2 && alice_zone.total == 2 && alice_zone.milestone_100,
		"zone completion did not use distinct definitions");
	const personal_summary alice = tracker.summary_for(7, 42);
	require(alice.completed == 2 && alice.total == 4 && alice.full_zones == 1,
		"personal summary was incorrect");
	const personal_summary bob = tracker.summary_for(7, 77);
	require(bob.completed == 2 && bob.total == 4,
		"group participant did not receive distinct completion credit");

	const leaderboard_page board = tracker.leaderboard(7, 0, 0, 10, 42);
	require(board.total_entries == 3 && board.own_rank == 1,
		"overall leaderboard omitted offline or own rank");
	require(board.entries[0].completed == 2 && board.entries[0].total == 4,
		"leaderboard did not expose exact completion values");
	require(board.entries[0].rank == board.entries[1].rank,
		"equal exact values did not share a rank");
	const std::string leaderboard_output = tracker.render_leaderboard(7, 0, 0, 10, 42, true);
	const std::string plain_leaderboard = tracker.render_leaderboard(7, 0, 0, 10, 42, false);
	const std::string hidden_zone_leaderboard =
		tracker.render_leaderboard(7, 900, 0, 10, 42, false);
	require(leaderboard_output.find("&+Y* ") != std::string::npos &&
			leaderboard_output.find("50%") != std::string::npos &&
			leaderboard_output.find("PID") == std::string::npos &&
			leaderboard_output.find("900") == std::string::npos &&
			plain_leaderboard.find("* #1") != std::string::npos &&
			plain_leaderboard.find("full zones") == std::string::npos &&
			hidden_zone_leaderboard == plain_leaderboard &&
			plain_leaderboard.find('&') == std::string::npos,
		"leaderboard output did not enforce the worldwide player-facing format");

	const std::string color_output = tracker.render_zone(7, 42, 900, "Alice", true);
	const std::string plain_output = tracker.render_zone(7, 42, 900, "Alice", false);
	require(color_output.find("&+") != std::string::npos &&
			plain_output.find("The Ember Coast") != std::string::npos &&
			plain_output.find("Zone 900") == std::string::npos &&
			plain_output.find('&') == std::string::npos,
		"achievement renderer did not honor color preference");
	const std::string summary_output = tracker.render_summary(7, 42, "Alice", false);
	require(summary_output.find("The Ember Coast") != std::string::npos &&
			summary_output.find("The Dusk Archive") != std::string::npos &&
			summary_output.find("Zone 900") == std::string::npos,
		"personal achievement summary did not use proper area names");

	daily_policy policy;
	policy.enabled = true;
	policy.minimum_attempts = 2;
	policy.minimum_distinct_pids = 2;
	policy.minimum_successes = 2;
	tracker.set_daily_policy(policy);
	for (uint32_t pid : { 100U, 101U })
	{
		telemetry_observation observation;
		observation.observation_id = "evidence-" + std::to_string(pid);
		observation.quest_definition_id = "zone-story:901:002";
		observation.content_revision = 7;
		observation.observed_at = 172800400 + pid;
		observation.pid = pid;
		observation.level = 10;
		observation.racewar = 1;
		observation.outcome = telemetry_outcome::success;
		observation.accessible = true;
		observation.party_size = 1;
		observation.strongest_party_level = 10;
		require(tracker.record_telemetry(observation, &error) == result::applied,
			"telemetry evidence was not recorded");
	}
	const evidence_summary evidence = tracker.evidence_for("zone-story:901:002", 7);
	require(evidence.suitable && evidence.observed_attempts == 2 && evidence.distinct_pids == 2,
		"evidence policy did not produce a suitable candidate");
	const int64_t daily_now = 200 * 86400 + 100;
	const daily_assignment assignment = tracker.assign_daily(7, 42, 10, 1, daily_now, &error);
	require(assignment.status == daily_status::assigned &&
			assignment.quest_definition_id == "zone-story:901:002",
		"daily assignment did not select the evidence-backed quest");
	require(tracker.assign_daily(7, 42, 10, 1, daily_now, &error).quest_definition_id ==
			assignment.quest_definition_id,
		"daily assignment rerolled within a fixed period");
	const std::string daily_score = tracker.render_daily_score(7, 42, 10, 1, daily_now, false);
	require(daily_score.find("quest available") != std::string::npos &&
			daily_score.find("Objective") == std::string::npos &&
			daily_score.find("The Dusk Archive") == std::string::npos,
		"score exposed more than the minimum daily reminder");
	const std::string daily_detail = tracker.render_daily(7, 42, 10, 1, daily_now, false);
	require(daily_detail.find("Carry the dusk message") != std::string::npos &&
			daily_detail.find("The Dusk Archive") != std::string::npos &&
			daily_detail.find("dusk archivist") != std::string::npos &&
			daily_detail.find("completion_key") == std::string::npos,
		"daily quest detail was not player-facing");

	completion_event daily_completion = completion("tx-daily", "zone-story:901:002", 901, 42,
						       assignment.assigned_at + 100, { 42 });
	require(tracker.record_completion(daily_completion, &error) == result::applied,
		"daily authoritative completion was not applied");
	require(tracker.complete_daily(7, 42, "tx-daily", daily_completion.transaction.completed_at,
				       &error) == result::already_applied,
		"daily reward was not finalized exactly once");
	require(tracker.summary_for(7, 42).renown == 1,
		"daily completion awarded the wrong renown amount");
	require(tracker.summary_for(7, 42).completed == 3,
		"daily repeat completion incorrectly changed zone numerator more than once");
	require(tracker.render_daily(7, 42, 10, 1, daily_now, false).find("Renown: 1") !=
			std::string::npos,
		"daily output did not expose the committed renown total");

	const std::string serialized = tracker.serialize_state(&error);
	service recovered(catalog);
	recovered.set_daily_policy(policy);
	require(recovered.deserialize_state(serialized, &error), "state did not recover");
	require(recovered.summary_for(7, 42).renown == 1 &&
			recovered.progress_for_zone(7, 42, 900).completed == 2,
		"recovered state lost completion or renown");
	require(recovered.daily_for(7, 42, service::period_for(daily_now)).status ==
			daily_status::completed,
		"recovered daily assignment did not remain completed");

	recovered.remember_character(8, 42, "OldSeasonName");
	telemetry_observation deleted_observation;
	deleted_observation.observation_id = "deleted-pid-observation";
	deleted_observation.quest_definition_id = "zone-story:900:001";
	deleted_observation.content_revision = 7;
	deleted_observation.observed_at = daily_now + 1000;
	deleted_observation.pid = 42;
	deleted_observation.level = 10;
	deleted_observation.racewar = 1;
	deleted_observation.outcome = telemetry_outcome::success;
	deleted_observation.accessible = true;
	require(recovered.record_telemetry(deleted_observation, &error) == result::applied,
		"deletion telemetry fixture was not applied");
	require(recovered.erase_character_all_seasons(42, 9),
		"all-season character deletion tombstone was not applied");
	require(recovered.summary_for(7, 42).completed == 0 &&
			recovered.summary_for(8, 42).character_name.empty() &&
			recovered.evidence_for("zone-story:900:001", 7).observed_attempts == 0,
		"all-season deletion retained personal or telemetry state");
	const std::string deleted_serialized = recovered.serialize_state(&error);
	require(deleted_serialized.find("X|7|42") != std::string::npos &&
			deleted_serialized.find("X|8|42") != std::string::npos &&
			deleted_serialized.find("X|9|42") != std::string::npos &&
			deleted_serialized.find("C|7|42|") == std::string::npos,
		"all-season deletion did not persist PID tombstones");
	service recovered_after_delete(catalog);
	recovered_after_delete.set_daily_policy(policy);
	require(recovered_after_delete.deserialize_state(deleted_serialized, &error),
		"state did not recover after all-season deletion");
	require(recovered_after_delete.summary_for(7, 42).completed == 0 &&
			recovered_after_delete.summary_for(7, 77).completed == 2 &&
			recovered_after_delete.evidence_for("zone-story:900:001", 7)
					.observed_attempts == 0,
		"deletion tombstone did not survive restart without erasing a group member");

	service disabled(catalog);
	require(disabled.get_daily_policy().enabled == false,
		"daily quests were not disabled by default");
	require(disabled.render_daily(7, 42, 10, 1, daily_now, false).empty() &&
			disabled.render_daily_score(7, 42, 10, 1, daily_now, false).empty(),
		"disabled daily output exposed the feature");
	require(disabled.daily_for(7, 42, service::period_for(daily_now)).status ==
			daily_status::none,
		"disabled daily rendering created durable assignment state");
	service disabled_after_assignment(catalog);
	disabled_after_assignment.set_daily_policy(policy);
	for (uint32_t pid : { 100U, 101U })
	{
		telemetry_observation observation;
		observation.observation_id = "disabled-evidence-" + std::to_string(pid);
		observation.quest_definition_id = "zone-story:901:002";
		observation.content_revision = 7;
		observation.observed_at = 172800500 + pid;
		observation.pid = pid;
		observation.level = 10;
		observation.racewar = 1;
		observation.party_size = 1;
		observation.strongest_party_level = 10;
		observation.outcome = telemetry_outcome::success;
		observation.accessible = true;
		require(disabled_after_assignment.record_telemetry(observation, &error) ==
				result::applied,
			"disabled reward evidence fixture was not applied");
	}
	const daily_assignment disabled_fixture =
		disabled_after_assignment.assign_daily(7, 42, 10, 1, daily_now, &error);
	disabled_after_assignment.set_daily_policy(daily_policy{});
	completion_event disabled_completion =
		completion("tx-disabled", disabled_fixture.quest_definition_id.c_str(), 901, 42,
			   disabled_fixture.assigned_at + 100, { 42 });
	require(disabled_after_assignment.record_completion(disabled_completion, &error) ==
				result::applied &&
			disabled_after_assignment.summary_for(7, 42).renown == 0,
		"disabling daily quests allowed a new reward to be earned");

	zone_story_quest_catalog::catalog empty_catalog = { .content_revision = 7,
							    .definitions = {} };
	service no_quests(empty_catalog);
	require(no_quests.render_summary(7, 42, "Alice", false).find("N/A") != std::string::npos,
		"no-quest catalog did not render explicit N/A");

	std::cout << "zone-story quest feature domain regression passed\n";
	return 0;
}
