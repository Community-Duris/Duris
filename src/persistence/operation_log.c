#include "persistence/operation_log.h"
#include "player/player_save_worker.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <memory>
#include <mutex>

namespace
{
enum class admission : uint8_t
{
	pending,
	durable,
	aborted,
	uncertain
};
struct retained_operation
{
	critical_command command;
	critical_apply_fn project;
	void *context;
	std::atomic<admission> state{ admission::pending };
};

std::mutex admission_mutex;
critical_apply_fn projector = nullptr;
void *projector_context = nullptr;
std::atomic<bool> ready{ false };
std::atomic<size_t> pending{ 0 };
uint64_t writer_owner = 0;

bool replay(critical_command command, void *)
{
	// Never load a partly recovered world. A failure leaves this record and
	// every later record intact for the next recovery attempt.
	const auto result = projector(command, projector_context);
	return operation_log_result_matches(command, result) &&
	       critical_command_journal_checkpoint(command.operation_id) ==
		       critical_command_journal_result::ok;
}

player_save_apply_result project(const std::shared_ptr<retained_operation> &operation)
{
	const auto state = operation->state.load();
	if (state == admission::aborted)
		return { player_save_apply_outcome::terminal_failure, 0, EIO };
	if (state != admission::durable)
		return { player_save_apply_outcome::retryable_failure, 0, EAGAIN };
	critical_apply_result result{};
	try
	{
		result = operation->project(operation->command, operation->context);
	}
	catch (...)
	{
		return { player_save_apply_outcome::retryable_failure, 0, ENOMEM };
	}
	if (!operation_log_result_matches(operation->command, result))
	{
		if (result.outcome != critical_apply_outcome::retryable_failure &&
		    result.outcome != critical_apply_outcome::ambiguous_commit)
			ready = false;
		// Do not deliver a rejection to gameplay or let a following save pass
		// a durable operation that the materialized base cannot reproduce.
		return { player_save_apply_outcome::retryable_failure, 0,
			 result.error_code ? result.error_code : EILSEQ };
	}
	if (critical_command_journal_checkpoint(operation->command.operation_id) !=
	    critical_command_journal_result::ok)
		return { player_save_apply_outcome::retryable_failure, 0, EIO };
	--pending;
	return { player_save_apply_outcome::applied, result.durable_revision, 0 };
}
} // namespace

bool operation_log_result_matches(const critical_command &command,
				  const critical_apply_result &result) noexcept
{
	return (result.outcome == critical_apply_outcome::applied ||
		result.outcome == critical_apply_outcome::already_applied) &&
	       !result.error_code &&
	       command.schema_version == CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION &&
	       result.durable_revision == command.outcome_revision &&
	       result.result_size == command.outcome_payload.size() &&
	       std::equal(command.outcome_payload.begin(), command.outcome_payload.end(),
			  result.result_payload.begin());
}

bool operation_log_init(const char *directory, critical_apply_fn callback, void *context)
{
	std::lock_guard<std::mutex> lock(admission_mutex);
	if (projector || !callback || !critical_command_journal_init(directory))
		return false;
	projector = callback;
	projector_context = context;
	try
	{
		ready = critical_command_journal_replay(replay, nullptr) ==
			critical_command_journal_result::ok;
	}
	catch (...)
	{
		ready = false;
	}
	return ready;
}

static critical_submit_result submit(const critical_command &command,
				     const player_snapshot *snapshot)
{
	std::lock_guard<std::mutex> lock(admission_mutex);
	if (!ready)
		return critical_submit_result::unavailable;
	if (command.schema_version != CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION ||
	    !critical_command_valid(command))
		return critical_submit_result::invalid;
	std::shared_ptr<retained_operation> operation;
	try
	{
		// Everything the projector will retain is allocated before the append.
		operation = std::make_shared<retained_operation>();
		operation->command = command;
		operation->project = projector;
		operation->context = projector_context;
		const size_t bytes =
			sizeof(retained_operation) + command.payload.size() +
			command.outcome_payload.size() +
			command.keys.size() * sizeof(critical_entity_key) +
			command.expected_revisions.size() * sizeof(critical_expected_revision);
		const auto submitted =
			snapshot ?
				player_save_worker_submit_logged(*snapshot, [operation]
								 { return project(operation); }) :
				persistence_writer_submit(persistence_job_kind::critical,
							  ++writer_owner, bytes, [operation]
							  { return project(operation); });
		if (submitted != player_save_submit_result::accepted)
			return critical_submit_result::overloaded;
	}
	catch (...)
	{
		return critical_submit_result::overloaded;
	}
	const auto appended = critical_command_journal_append(command);
	if (appended == critical_command_journal_result::ok)
	{
		++pending;
		operation->state = admission::durable;
		return critical_submit_result::accepted;
	}
	if (appended == critical_command_journal_result::append_uncertain)
	{
		// Restart/recovery resolves an ambiguous append. Neither gameplay nor
		// later database writes may escape this boundary in the current process.
		ready = false;
		++pending;
		operation->state = admission::uncertain;
		return critical_submit_result::journal_uncertain;
	}
	operation->state = admission::aborted;
	return critical_submit_result::journal_failure;
}

critical_submit_result operation_log_submit(const critical_command &command)
{
	return submit(command, nullptr);
}

critical_submit_result operation_log_submit_player(const critical_command &command,
						   const player_snapshot &snapshot)
{
	return submit(command, &snapshot);
}

bool operation_log_ready()
{
	return ready;
}
bool operation_log_projection_caught_up()
{
	return ready && !pending.load();
}

void operation_log_shutdown()
{
	std::lock_guard<std::mutex> lock(admission_mutex);
	ready = false;
	projector = nullptr;
	projector_context = nullptr;
	pending = 0;
	critical_command_journal_shutdown();
}
