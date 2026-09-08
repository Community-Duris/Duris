#include "world/zone_story_quest_repository.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

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

zone_story_quest_tracking::completion_transaction transaction(const char *id, uint32_t direct_pid,
							      std::vector<uint32_t> credited)
{
	return { .schema_version =
			 zone_story_quest_tracking::ZONE_STORY_QUEST_TRACKING_SCHEMA_VERSION,
		 .transaction_id = id,
		 .quest_definition_id = "zone-story:900:001",
		 .zone_number = 900,
		 .direct_completer_pid = direct_pid,
		 .credited_pids = std::move(credited),
		 .room_vnum = 90001,
		 .completed_at = 1700000000,
		 .season_id = 3,
		 .content_revision = 7 };
}
} // namespace

int main()
{
	using namespace zone_story_quest_repository;
	in_memory_completion_repository repository;
	std::string error;

	const auto solo = transaction("tx-solo", 42, { 42 });
	require(repository.record(solo, &error) == record_result::applied,
		"first transaction was not applied");
	require(repository.record(solo, &error) == record_result::already_applied,
		"replayed transaction was not idempotent");
	require(repository.size() == 1, "replayed transaction duplicated storage");

	zone_story_quest_tracking::completion_transaction loaded = {};
	require(repository.get_transaction("tx-solo", &loaded, &error),
		"stored transaction could not be loaded");
	require(loaded.transaction_id == solo.transaction_id &&
			loaded.credited_pids == solo.credited_pids,
		"loaded transaction did not match stored transaction");

	const auto group = transaction("tx-group", 42, { 42, 77 });
	require(repository.record(group, &error) == record_result::applied,
		"group transaction was not applied");
	require(repository.list_for_pid(77, 3, 900).size() == 1,
		"PID/season/zone query did not find group credit");
	require(repository.list_for_pid(77, 4, 900).empty(), "season filter was ignored");
	require(repository.list_for_pid(77, 3, 901).empty(), "zone filter was ignored");

	auto conflict = solo;
	conflict.room_vnum = 90002;
	require(repository.record(conflict, &error) == record_result::conflict,
		"same transaction ID with different data was accepted");

	auto invalid = group;
	invalid.transaction_id = "tx-invalid";
	invalid.credited_pids = { 42, 77, 77 };
	require(repository.record(invalid, &error) == record_result::invalid,
		"invalid transaction was stored");
	require(repository.size() == 2, "invalid/conflicting transaction changed storage");

	std::cout << "zone-story quest repository contract regression passed\n";
	return 0;
}
