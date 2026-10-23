#include "world/zone_story_quest_feature.h"
#include "flatfile/flatfile_zone_story_quest_state.h"
#include <cassert>
#include <array>
#ifdef ZSQ_TEST_SQL
#include "sql/zone_story_quest_state_repository.h"
#include "zone_story_quest_sql_test_support.h"
#endif
#include <chrono>
#include <filesystem>
#include <iostream>

int main(int argc, char **argv)
{
#ifdef ZSQ_TEST_SQL
	assert(argc == 4 && std::string(argv[3]).starts_with("zone_daily_"));
	DB = mysql_init(nullptr);
	assert(mysql_real_connect(DB, "localhost", "root", "zone-daily-disposable", argv[3], 0,
				  argv[2], 0));
	assert(!mysql_query(DB, "DELETE FROM zone_story_quest_state"));
	std::string initial;
	assert(sql_zone_story_quest_state_load(2, &initial) ==
	       sql_zone_story_quest_state_result::not_found);
#else
	assert(argc == 2);
#endif
	const auto root = std::filesystem::path(argv[1]);
	std::filesystem::create_directories(root / "domains");
	std::filesystem::permissions(root, std::filesystem::perms::owner_all);
	std::filesystem::permissions(root / "domains", std::filesystem::perms::owner_all);
	double worst_ms = 0, total_ms = 0;
	size_t writes = 0;
	using namespace zone_story_quest_feature;
	zone_story_quest_catalog::catalog catalog;
	catalog.content_revision = 2;
	catalog.zones = { { 831, "Alatorin", "alatorin", 83100, 84055, true } };
	for (int index = 0; index < 4; ++index)
	{
		zone_story_quest_tracking::quest_definition definition;
		definition.definition_id =
			"zone-story:qst:83101:" + std::string(180, 'a') + std::to_string(index);
		definition.source_system = "zone_story";
		definition.source_area = "alatorin";
		definition.zone_number = 831;
		definition.giver_vnum = 83101;
		definition.completion_key = definition.definition_id;
		definition.active = definition.repeatable =
			definition.eligible_for_zone_completion = definition.daily_eligible = true;
		definition.content_revision = 2;
		catalog.definitions.push_back(definition);
	}
	service tracker(catalog);
	daily_policy policy;
	policy.enabled = true;
	tracker.set_daily_policy(policy);
	for (uint32_t pid = 1; pid <= 25; ++pid)
	{
		tracker.remember_character(7, pid, "Capacity" + std::to_string(pid));
		assert(tracker.discover_zone(7, pid, 831, 83450, 864000, "arrival") ==
		       result::applied);
		for (int day = 0; day < 60; ++day)
			for (const auto &definition : catalog.definitions)
			{
				completion_event event;
				event.transaction.schema_version = 2;
				event.transaction.transaction_id =
					"offering:" + std::to_string(pid) + ":" +
					std::to_string(day) + ":" + definition.definition_id;
				event.transaction.quest_definition_id = definition.definition_id;
				event.transaction.zone_number = 831;
				event.transaction.direct_completer_pid = pid;
				event.transaction.credited_pids = { pid };
				event.transaction.daily_policy_revision = 1;
				event.transaction.daily_credited_pids = { pid };
				event.transaction.room_vnum = 83450;
				event.transaction.completed_at = 864000 + day * 86400 + 1;
				event.transaction.season_id = 7;
				event.transaction.content_revision = 2;
				event.level = 10;
				event.racewar = 1;
				event.party_context_known = true;
				event.strongest_party_level = 10;
				const auto start = std::chrono::steady_clock::now();
				const auto undo = tracker.checkpoint_for(
					7, { pid }, event.transaction.transaction_id);
				assert(tracker.record_completion(event) == result::applied);
#ifdef ZSQ_TEST_SQL
				assert(sql_zone_story_quest_records_save(
					       2, tracker.changes_for_persistence()) ==
				       sql_zone_story_quest_state_result::ok);
#else
				assert(flatfile_zone_story_quest_records_save(
					       argv[1], 2, tracker.changes_for_persistence()) ==
				       flatfile_zone_story_quest_result::ok);
#endif
				tracker.mark_persisted();
				const double ms = std::chrono::duration<double, std::milli>(
							  std::chrono::steady_clock::now() - start)
							  .count();
				worst_ms = std::max(worst_ms, ms);
				total_ms += ms;
				++writes;
			}
	}
	const auto begin = std::chrono::steady_clock::now();
	const std::string state = tracker.serialize_state();
	const auto serialized = std::chrono::steady_clock::now();
	const auto saved = std::chrono::steady_clock::now();
	std::string loaded;
#ifdef ZSQ_TEST_SQL
	assert(sql_zone_story_quest_state_load(2, &loaded) ==
	       sql_zone_story_quest_state_result::ok);
#else
	assert(flatfile_zone_story_quest_state_load(argv[1], 2, &loaded) ==
		       flatfile_zone_story_quest_result::ok &&
	       !loaded.empty());
#endif
	service restored(catalog);
	assert(restored.deserialize_state(loaded) && restored.summary_for(7, 1).renown == 60);
	assert(state.size() > 16U * 1024U * 1024U);
	assert(restored.serialize_state() == state);
	const auto records = zone_story_quest_state::split_document(state);
	std::array<zone_story_quest_state::records, 256> buckets;
	for (const auto &[key, value] : records)
		buckets[zone_story_quest_state::bucket(key)][key] = value;
	size_t maximum_bucket = 0;
	for (const auto &bucket : buckets)
		maximum_bucket =
			std::max(maximum_bucket, zone_story_quest_state::encode(bucket).size());
	assert(maximum_bucket < 16U * 1024U * 1024U);
	size_t stored_bytes = 0, maximum_stored = maximum_bucket;
#ifdef ZSQ_TEST_SQL
	assert(!mysql_query(
		DB,
		"SELECT MAX(OCTET_LENGTH(state_blob)),SUM(OCTET_LENGTH(state_blob)) FROM zone_story_quest_state"));
	MYSQL_RES *sizes = mysql_store_result(DB);
	assert(sizes);
	MYSQL_ROW row = mysql_fetch_row(sizes);
	assert(row && row[0] && row[1]);
	maximum_stored = std::stoull(row[0]);
	stored_bytes = std::stoull(row[1]);
	assert(maximum_stored < 16U * 1024U * 1024U);
	mysql_free_result(sizes);
#else
	stored_bytes = std::filesystem::file_size(root / "domains/zone-story-quests.state");
#endif
	std::cout <<
#ifdef ZSQ_TEST_SQL
		"backend=sql "
#else
		"backend=flatfile "
#endif
		"characters=25 days=60 completions=6000 bytes="
		  << state.size() << " serialize_ms="
		  << std::chrono::duration<double, std::milli>(serialized - begin).count()
		  << " delta_average_ms=" << total_ms / writes << " delta_worst_ms=" << worst_ms
		  << " max_uncompressed_bucket_bytes=" << maximum_bucket
		  << " max_sql_bucket_bytes=" << maximum_stored << " journal_bytes=" << stored_bytes
		  << " final_save_ms="
		  << std::chrono::duration<double, std::milli>(saved - serialized).count() << '\n';
}
