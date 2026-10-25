#include "persistence/critical_command_coordinator.h"
#include "player/player_save_worker.h"

#include <cassert>
#include <cerrno>

namespace
{
unsigned int prepared = 0, appended = 0;
critical_submit_result admission = critical_submit_result::accepted;
bool reject = false;

critical_command request(size_t bytes = 1)
{
	critical_command command{};
	command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	assert(critical_operation_id_generate(&command.operation_id));
	command.type = critical_command_type::test;
	command.payload_version = 1;
	command.source_site = critical_source_site::command;
	command.deadline_class = critical_deadline_class::interactive;
	command.keys = { { critical_entity_type::player, 17 } };
	command.payload.resize(bytes, 1);
	return command;
}

critical_apply_result prepare(const critical_command &, void *)
{
	++prepared;
	if (reject)
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	critical_apply_result result{ critical_apply_outcome::applied, 19, 0 };
	result.result_size = 1;
	result.result_payload[0] = 31;
	return result;
}

critical_submit_result append(const critical_command &command)
{
	++appended;
	assert(command.schema_version == CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION);
	assert(command.outcome_revision == 19 &&
	       command.outcome_payload == std::vector<uint8_t>{ 31 });
	return admission;
}

critical_receipt_status lookup(const critical_command &, critical_apply_result *)
{
	return critical_receipt_status::missing;
}

void init()
{
	prepared = appended = 0;
	reject = false;
	admission = critical_submit_result::accepted;
	assert(critical_command_coordinator_init_ram(prepare, nullptr, append, lookup));
}
}

player_save_submit_result persistence_writer_submit(persistence_job_kind, uint64_t, size_t,
						    persistence_job_write_fn)
{
	assert(false); // RAM admission calls its durable log, never the legacy writer adapter.
	return player_save_submit_result::unavailable;
}

int main()
{
	init();
	const auto first = request();
	const auto second = request();
	assert(critical_command_coordinator_submit_for_publication(first) ==
	       critical_submit_result::awaiting_durability);
	assert(critical_command_coordinator_submit_for_publication(second) ==
	       critical_submit_result::awaiting_durability);
	assert(prepared == 0 && appended == 0);
	critical_completion completion{};
	assert(critical_command_coordinator_pulse(nullptr, 0) == 0);
	assert(prepared == 0 && appended == 0);
	for (auto failure :
	     { critical_submit_result::overloaded, critical_submit_result::journal_failure,
	       critical_submit_result::unavailable })
	{
		admission = failure;
		assert(critical_command_coordinator_pulse(&completion, 1) == 0);
		assert(critical_command_coordinator_durability(first.operation_id) ==
		       critical_command_durability::awaiting_durability);
		critical_operation_id owner{};
		assert(critical_command_coordinator_is_fenced(first.keys[0], &owner));
		assert(critical_operation_id_equal(owner, first.operation_id));
		assert(!critical_command_coordinator_get_completed(first.operation_id,
								   &completion));
	}
	admission = critical_submit_result::accepted;
	assert(critical_command_coordinator_pulse(&completion, 1) == 1);
	assert(completion.result_payload[0] == 31 && completion.durable_revision == 19);
	const auto calls = prepared;
	assert(critical_command_coordinator_pulse(&completion, 1) == 0);
	assert(prepared == calls); // Publication, not just append, releases the next RAM decision.
	assert(critical_command_coordinator_acknowledge_publication(first.operation_id));
	reject = true;
	assert(critical_command_coordinator_pulse(&completion, 1) == 1);
	assert(completion.outcome == critical_apply_outcome::terminal_failure &&
	       completion.error_code == EINVAL);
	assert(appended == calls); // A rejected decision never becomes an accepted log record.
	assert(critical_command_coordinator_acknowledge_publication(second.operation_id));
	assert(!critical_command_coordinator_is_fenced(first.keys[0], nullptr));
	critical_command_coordinator_shutdown();

	// Count and byte limits are enforced before preparation or durable I/O.
	for (size_t bytes : { size_t{ 1 }, CRITICAL_COMMAND_MAX_PAYLOAD_BYTES })
	{
		init();
		size_t accepted = 0;
		for (;;)
		{
			const auto result = critical_command_coordinator_submit(request(bytes));
			if (result == critical_submit_result::overloaded)
				break;
			assert(result == critical_submit_result::awaiting_durability);
			assert(++accepted <= CRITICAL_COORDINATOR_MAX_OPERATIONS);
		}
		const auto health = critical_command_coordinator_health_copy();
		assert(health.inflight == accepted && health.awaiting_durability == accepted);
		assert(health.retained_bytes <= CRITICAL_COORDINATOR_MAX_BYTES);
		assert(prepared == 0 && appended == 0);
		if (bytes == 1)
			assert(accepted == CRITICAL_COORDINATOR_MAX_OPERATIONS);
		else
			assert(accepted < CRITICAL_COORDINATOR_MAX_OPERATIONS);
		critical_command_coordinator_shutdown();
	}

	init();
	const auto uncertain = request();
	assert(critical_command_coordinator_submit(uncertain) ==
	       critical_submit_result::awaiting_durability);
	admission = critical_submit_result::journal_uncertain;
	assert(critical_command_coordinator_pulse(&completion, 1) == 0);
	critical_command_coordinator_resume();
	assert(critical_command_coordinator_submit(request()) ==
	       critical_submit_result::unavailable);
	assert(critical_command_coordinator_durability(uncertain.operation_id) ==
	       critical_command_durability::uncertain);
	assert(critical_command_coordinator_is_fenced(uncertain.keys[0], nullptr));
	assert(critical_command_coordinator_pulse(&completion, 1) == 0);
	assert(prepared == 1 && appended == 1);
	critical_command_coordinator_shutdown();
}
