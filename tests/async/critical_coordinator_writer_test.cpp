#include "persistence/critical_command_coordinator.h"
#include "player/player_save_worker.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
std::atomic<bool> available{ true };
std::atomic<unsigned> attempts{ 0 };
std::mutex written_mutex;
std::vector<unsigned> written;

critical_command command(unsigned tag, uint64_t owner)
{
	critical_command result{};
	result.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	assert(critical_operation_id_generate(&result.operation_id));
	result.type = critical_command_type::test;
	result.payload_version = 1;
	result.source_site = critical_source_site::command;
	result.deadline_class = critical_deadline_class::interactive;
	result.keys = { { critical_entity_type::player, owner } };
	result.payload = { static_cast<uint8_t>(tag) };
	return result;
}

critical_apply_result apply(const critical_command &value, void *)
{
	++attempts;
	if (!available)
		return { critical_apply_outcome::ambiguous_commit, 0, 2013 };
	if (value.payload[0] == 20)
	{
		critical_apply_result rejected{ critical_apply_outcome::terminal_failure, 0,
						ESTALE };
		rejected.failure_stage = critical_failure_stage::coin_source_wallet_revision;
		return rejected;
	}
	{
		std::lock_guard<std::mutex> lock(written_mutex);
		written.push_back(value.payload[0]);
	}
	critical_apply_result result{ critical_apply_outcome::applied, 1, 0 };
	result.result_size = 1;
	result.result_payload[0] = value.payload[0];
	return result;
}

player_save_apply_result snapshot(const player_snapshot &, void *)
{
	assert(false);
	return {};
}

template <typename Predicate> void wait_until(Predicate predicate)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (!predicate())
	{
		assert(std::chrono::steady_clock::now() < deadline);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void reap_writer()
{
	player_save_completion completion[64]{};
	while (player_save_worker_pulse(completion, 64))
	{
	}
}
}

int main()
{
	assert(player_save_worker_init(snapshot, nullptr));
	assert(critical_command_coordinator_init(apply, nullptr));
	const auto first = command(1, 1), follower = command(2, 1), unrelated = command(3, 2);
	assert(critical_command_coordinator_submit_for_publication(first) ==
	       critical_submit_result::accepted);
	assert(critical_command_coordinator_submit_for_publication(first) ==
	       critical_submit_result::attached);
	assert(critical_command_coordinator_submit(first) ==
	       critical_submit_result::identity_conflict);
	auto conflict = first;
	conflict.payload[0] = 99;
	assert(critical_command_coordinator_submit_for_publication(conflict) ==
	       critical_submit_result::identity_conflict);
	critical_completion result[16]{};
	wait_until([&] { return critical_command_coordinator_pulse(result, 16) == 1; });
	assert(result[0].result_payload[0] == 1);
	assert(critical_command_coordinator_health_copy().publication_pending == 1);
	assert(critical_command_coordinator_is_fenced({ critical_entity_type::player, 1 },
						      nullptr));
	assert(!critical_command_coordinator_drain(5));
	assert(critical_command_coordinator_submit(follower) == critical_submit_result::accepted);
	assert(critical_command_coordinator_submit(unrelated) == critical_submit_result::accepted);
	// The one writer preserves capture order even across disjoint owners. The
	// follower cannot execute against a committed but unpublished predecessor.
	wait_until([] { return player_save_worker_health_copy().connection_retries > 0; });
	assert(attempts == 1);
	assert(critical_command_coordinator_pulse(result, 16) == 0);
	assert(critical_command_coordinator_acknowledge_publication(first.operation_id));
	assert(critical_command_coordinator_drain(3000));
	assert(persistence_writer_wait_idle(3000));
	assert((written == std::vector<unsigned>{ 1, 2, 3 }));
	assert(!critical_command_coordinator_is_fenced({ critical_entity_type::player, 1 },
						       nullptr));
	assert(critical_command_coordinator_submit(first) == critical_submit_result::attached);
	reap_writer();

	// An ambiguous result retains the queue head; neither a later command nor
	// an ordinary snapshot/materialization job may overtake it.
	available = false;
	const auto retry = command(4, 4), after = command(5, 5);
	const auto prior_attempts = attempts.load();
	assert(critical_command_coordinator_submit(retry) == critical_submit_result::accepted);
	assert(critical_command_coordinator_submit(after) == critical_submit_result::accepted);
	assert(persistence_writer_submit(persistence_job_kind::saved_item, 1, 1,
					 []
					 {
						 std::lock_guard<std::mutex> lock(written_mutex);
						 written.push_back(6);
						 return player_save_apply_result{
							 player_save_apply_outcome::applied, 1, 0
						 };
					 }) == player_save_submit_result::accepted);
	wait_until([&] { return attempts > prior_attempts; });
	assert(critical_command_coordinator_pulse(result, 16) == 0);
	assert(critical_command_coordinator_durability(retry.operation_id) ==
	       critical_command_durability::awaiting_durability);
	available = true;
	assert(critical_command_coordinator_drain(3000));
	assert(persistence_writer_wait_idle(3000));
	assert((written == std::vector<unsigned>{ 1, 2, 3, 4, 5, 6 }));
	assert(critical_command_coordinator_health_copy().ambiguous > 0);
	reap_writer();

	critical_completion stale{};
	stale.operation_id = retry.operation_id;
	stale.attempt = 99;
	assert(critical_command_coordinator_inject_completion_for_tests(stale));
	assert(critical_command_coordinator_pulse(result, 16) == 0);
	assert(critical_command_coordinator_health_copy().stale_completions == 1);
	assert(critical_command_coordinator_submit(command(20, 20)) ==
	       critical_submit_result::accepted);
	wait_until([&] { return critical_command_coordinator_pulse(result, 16) == 1; });
	assert(result[0].outcome == critical_apply_outcome::terminal_failure &&
	       result[0].error_code == ESTALE &&
	       result[0].failure_stage == critical_failure_stage::coin_source_wallet_revision);
	assert(!critical_command_coordinator_is_fenced({ critical_entity_type::player, 20 },
						       nullptr));

	// The coordinator bound includes all commands awaiting publication.
	for (size_t index = 0; index < CRITICAL_COORDINATOR_MAX_OPERATIONS; ++index)
		assert(critical_command_coordinator_submit_for_publication(
			       command(7, 100 + index)) == critical_submit_result::accepted);
	assert(critical_command_coordinator_submit(command(8, 9999)) ==
	       critical_submit_result::overloaded);
	unsigned delivered = 0;
	wait_until(
		[&]
		{
			delivered += critical_command_coordinator_pulse(result, 16);
			reap_writer();
			return delivered == CRITICAL_COORDINATOR_MAX_OPERATIONS;
		});
	assert(critical_command_coordinator_health_copy().publication_pending ==
	       CRITICAL_COORDINATOR_MAX_OPERATIONS);
	assert(persistence_writer_wait_idle(3000));
	critical_command_coordinator_quiesce();
	assert(critical_command_coordinator_submit(command(9, 9999)) ==
	       critical_submit_result::unavailable);
	player_save_worker_shutdown();
	critical_command_coordinator_shutdown();
	player_save_worker_reset_for_tests();
	assert(player_save_worker_init(snapshot, nullptr));
	assert(critical_command_coordinator_init(apply, nullptr));
	assert(critical_command_coordinator_health_copy().publication_pending == 0);
	assert(critical_command_coordinator_submit(command(10, 1)) ==
	       critical_submit_result::accepted);
	assert(critical_command_coordinator_drain(3000));
	player_save_worker_shutdown();
	critical_command_coordinator_shutdown();
	player_save_worker_reset_for_tests();
}
