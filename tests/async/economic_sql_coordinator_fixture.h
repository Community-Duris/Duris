#ifndef DURIS_TEST_ECONOMIC_SQL_COORDINATOR_FIXTURE_H
#define DURIS_TEST_ECONOMIC_SQL_COORDINATOR_FIXTURE_H

#include "economy/economic_command_admission.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/critical_command_repository.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

inline critical_apply_result
exercise_sql_coordinator(critical_command command, const char *family,
			 bool lose_commit_reply = false,
			 critical_apply_outcome first_outcome = critical_apply_outcome::applied)
{
	assert(economic_command_admission_supported(command));
	std::string directory = std::string("/tmp/duris-economic-") + family + "-XXXXXX";
	assert(mkdtemp(directory.data()));
	bool reply_pending = lose_commit_reply;
	const auto apply = [](const critical_command &submitted, void *context)
	{
		const auto result = critical_command_repository_apply_from_pool(submitted, nullptr);
		auto &pending = *static_cast<bool *>(context);
		if (pending && result.outcome == critical_apply_outcome::applied)
		{
			pending = false;
			return critical_apply_result{ critical_apply_outcome::ambiguous_commit, 0,
						      2013 };
		}
		return result;
	};
	const auto start = [&]
	{
		assert(critical_command_coordinator_init(directory.c_str(), apply, &reply_pending,
							 1, nullptr, nullptr,
							 economic_command_admission_supported));
	};
	const auto completion = [&]
	{
		critical_completion result = {};
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
		while (critical_command_coordinator_pulse(&result, 1) != 1)
		{
			assert(std::chrono::steady_clock::now() < deadline);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		assert(critical_operation_id_equal(result.operation_id, command.operation_id));
		assert(!result.error_code);
		return result;
	};
	start();
	assert(critical_command_coordinator_submit_for_publication(command) ==
	       critical_submit_result::awaiting_durability);
	const auto applied = completion();
	assert(applied.outcome ==
	       (lose_commit_reply ? critical_apply_outcome::already_applied : first_outcome));
	if (lose_commit_reply)
	{
		assert(!reply_pending);
		assert(critical_command_coordinator_health_copy().ambiguous == 1);
		assert(critical_command_coordinator_health_copy().retries == 1);
	}
	else
	{
		// Distinguish native pooled reconciliation from synthetic ambiguity.
		assert(critical_command_coordinator_health_copy().ambiguous == 0);
		assert(critical_command_coordinator_health_copy().retries == 0);
	}
	auto conflict = command;
	++conflict.accepted_at_usec;
	assert(critical_command_coordinator_submit_for_publication(conflict) ==
	       critical_submit_result::identity_conflict);
	assert(critical_command_journal_health_copy().checkpoints == 0);
	assert(critical_command_coordinator_health_copy().publication_pending == 1);
	assert(critical_command_coordinator_shutdown());
	start();
	const auto replayed = completion();
	assert(replayed.outcome == critical_apply_outcome::already_applied);
	assert(replayed.durable_revision == applied.durable_revision &&
	       replayed.result_size == applied.result_size &&
	       replayed.result_payload == applied.result_payload);
	assert(critical_command_journal_health_copy().checkpoints == 0);
	assert(critical_command_coordinator_acknowledge_publication(command.operation_id));
	assert(critical_command_journal_health_copy().checkpoints == 1);
	assert(critical_command_coordinator_shutdown());
	start();
	assert(critical_command_coordinator_health_copy().publication_pending == 0);
	assert(!critical_command_coordinator_acknowledge_publication(command.operation_id));
	assert(critical_command_coordinator_shutdown());
	std::filesystem::remove_all(directory);
	return { applied.outcome,	applied.durable_revision, applied.error_code,
		 applied.failure_stage, applied.result_size,	  applied.result_payload };
}

#endif
