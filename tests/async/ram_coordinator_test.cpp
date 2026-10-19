#include "persistence/critical_command_coordinator.h"
#include "persistence/operation_log.h"
#include "player/player_save_worker.h"

#include <cassert>
#include <cerrno>
#include <deque>
#include <filesystem>
#include <unistd.h>

namespace
{
std::deque<persistence_job_write_fn> jobs;
bool fail_sync = false;
bool fail_truncate = false;
bool capacity = true;
unsigned int preparations = 0;
unsigned int projections = 0;
uint8_t live = 0;
uint8_t stored = 0;

critical_command command(uint8_t delta)
{
	critical_command result{};
	result.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	assert(critical_operation_id_generate(&result.operation_id));
	result.type = critical_command_type::test;
	result.payload_version = 1;
	result.source_site = critical_source_site::command;
	result.deadline_class = critical_deadline_class::interactive;
	result.accepted_at_usec = 1700000000000000ULL;
	result.keys = { { critical_entity_type::player, 9 } };
	result.payload = { delta };
	assert(critical_command_normalize(&result));
	return result;
}

critical_apply_result prepare(const critical_command &value, void *)
{
	++preparations;
	critical_apply_result result{ critical_apply_outcome::applied, 1, 0 };
	result.result_size = 1;
	result.result_payload[0] = live + value.payload[0];
	return result;
}

critical_apply_result project(const critical_command &value, void *)
{
	++projections;
	stored = value.outcome_payload[0];
	critical_apply_result result{ critical_apply_outcome::applied, 1, 0 };
	result.result_size = 1;
	result.result_payload[0] = stored;
	return result;
}

void pump()
{
	while (!jobs.empty())
	{
		assert(jobs.front()().outcome != player_save_apply_outcome::retryable_failure);
		jobs.pop_front();
	}
}

void publish_batch(const critical_completion *batch, size_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		live = batch[i].result_payload[0];
		assert(critical_command_coordinator_acknowledge_publication(batch[i].operation_id));
	}
}
}

extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd)
{
	const int result = __real_fsync(fd);
	if (fail_sync)
	{
		fail_sync = false;
		errno = EIO;
		return -1;
	}
	return result;
}

extern "C" int __real_ftruncate(int, off_t);
extern "C" int __wrap_ftruncate(int fd, off_t length)
{
	if (fail_truncate)
	{
		fail_truncate = false;
		errno = EIO;
		return -1;
	}
	return __real_ftruncate(fd, length);
}

player_save_submit_result persistence_writer_submit(persistence_job_kind, uint64_t, size_t,
						    persistence_job_write_fn write)
{
	if (!capacity)
		return player_save_submit_result::unavailable;
	jobs.push_back(std::move(write));
	return player_save_submit_result::accepted;
}

player_save_submit_result player_save_worker_submit_logged(player_snapshot,
							   persistence_job_write_fn)
{
	assert(false);
	return player_save_submit_result::unavailable;
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	assert(operation_log_init(argv[1], project, nullptr));
	assert(critical_command_coordinator_init_ram(prepare, nullptr, operation_log_submit,
						     operation_log_lookup));
	const auto first = command(3);
	const auto second = command(5);
	assert(critical_command_coordinator_submit_for_publication(first) ==
	       critical_submit_result::awaiting_durability);
	assert(critical_command_coordinator_submit_for_publication(second) ==
	       critical_submit_result::awaiting_durability);
	critical_completion completion{};
	capacity = false;
	assert(critical_command_coordinator_pulse(&completion, 1) == 0);
	assert(live == 0 && projections == 0);
	assert(critical_command_journal_health_copy().records == 0);
	capacity = true;
	assert(critical_command_coordinator_pulse(&completion, 1) == 1);
	assert(completion.result_payload[0] == 3 && projections == 0);
	const auto before = preparations;
	assert(critical_command_coordinator_pulse(&completion, 1) == 0);
	assert(preparations == before); // Deferred publication keeps the next decision fenced.
	live = 3;
	assert(critical_command_coordinator_acknowledge_publication(first.operation_id));
	assert(critical_command_coordinator_pulse(&completion, 1) == 1);
	assert(completion.result_payload[0] == 8 && projections == 0);
	live = 8;
	assert(critical_command_coordinator_acknowledge_publication(second.operation_id));
	assert(critical_command_journal_health_copy().records == 2);
	pump();
	assert(stored == 8 && projections == 2);
	assert(operation_log_projection_caught_up());
	assert(critical_command_coordinator_submit(second) == critical_submit_result::attached);
	assert(critical_command_coordinator_pulse(&completion, 1) == 0);
	assert(jobs.empty());

	const auto uncertain = command(7);
	assert(critical_command_coordinator_submit(uncertain) ==
	       critical_submit_result::awaiting_durability);
	fail_sync = fail_truncate = true;
	assert(critical_command_coordinator_pulse(&completion, 1) == 0);
	assert(critical_command_coordinator_durability(uncertain.operation_id) ==
	       critical_command_durability::uncertain);
	assert(!operation_log_ready());
	assert(live == 8 && stored == 8);
	critical_operation_id fenced{};
	assert(critical_command_coordinator_is_fenced({ critical_entity_type::player, 9 },
						      &fenced));
	assert(critical_operation_id_equal(fenced, uncertain.operation_id));
	assert(!critical_command_coordinator_recover_uncertain());
	critical_command_coordinator_resume();
	assert(critical_command_coordinator_submit(command(1)) ==
	       critical_submit_result::unavailable);
	assert(jobs.size() == 1);
	assert(jobs.front()().outcome == player_save_apply_outcome::retryable_failure);
	assert(projections == 2);
	critical_command_coordinator_shutdown();
	jobs.clear(); // Simulated process exit: no writer is running in this harness.
	operation_log_shutdown();
	assert(operation_log_init(argv[1], project, nullptr));
	assert(stored == 15 && projections == 3);
	assert(critical_command_journal_health_copy().records == 0);
	assert(critical_command_coordinator_init_ram(prepare, nullptr, operation_log_submit,
						     operation_log_lookup));
	live = stored;
	// Evict all process-local receipts, then retry an earlier request. Neither
	// preparation nor publication may run again, even with a new arrival time.
	for (size_t index = 0; index <= CRITICAL_COORDINATOR_COMPLETED_CACHE_MAX; ++index)
	{
		assert(critical_command_coordinator_submit(command(0)) ==
		       critical_submit_result::awaiting_durability);
		assert(critical_command_coordinator_pulse(&completion, 1) == 1);
		assert(completion.result_payload[0] == 15);
		pump();
	}
	const auto prepared_before_duplicate = preparations;
	auto duplicate = first;
	duplicate.accepted_at_usec += 1000;
	assert(critical_command_coordinator_submit(duplicate) == critical_submit_result::attached);
	assert(critical_command_coordinator_get_completed(first.operation_id, &completion));
	assert(completion.outcome == critical_apply_outcome::already_applied);
	assert(completion.result_payload[0] == 3);
	assert(preparations == prepared_before_duplicate && live == 15 && jobs.empty());
	critical_command_coordinator_shutdown();
	operation_log_shutdown();
	assert(operation_log_init(argv[1], project, nullptr));
	assert(critical_command_coordinator_init_ram(prepare, nullptr, operation_log_submit,
						     operation_log_lookup));
	assert(critical_command_coordinator_submit(first) == critical_submit_result::attached);
	assert(critical_command_coordinator_get_completed(first.operation_id, &completion));
	assert(completion.result_payload[0] == 3 && preparations == prepared_before_duplicate);
	duplicate = first;
	duplicate.payload[0] = 99;
	assert(critical_command_coordinator_submit(duplicate) ==
	       critical_submit_result::identity_conflict);
	// A pulse may admit several dependent operations, but every next decision
	// observes the predecessor's live publication. Capacity still bounds work.
	for (unsigned i = 0; i < 3; ++i)
		assert(critical_command_coordinator_submit_for_publication(command(1)) ==
		       critical_submit_result::awaiting_durability);
	critical_completion batch[3]{};
	assert(critical_command_coordinator_pump(batch, 2, publish_batch, 1000000) == 2);
	assert(live == 17 && batch[0].result_payload[0] == 16 && batch[1].result_payload[0] == 17);
	assert(critical_command_coordinator_pump(batch, 3, publish_batch, 0) == 1);
	assert(live == 18);
	pump();
	critical_command_coordinator_shutdown();
	operation_log_shutdown();
}
