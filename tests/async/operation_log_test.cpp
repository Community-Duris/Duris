#include "persistence/operation_log.h"
#include "player/player_save_worker.h"
#include "player/player_checkpoint_command.h"

#include <cassert>
#include <cerrno>
#include <deque>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>

static std::deque<persistence_job_write_fn> jobs;
static bool capacity = true;
static int sync_failures = 0;
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd)
{
	if (sync_failures)
	{
		--sync_failures;
		errno = EIO;
		return -1;
	}
	return __real_fsync(fd);
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
							   persistence_job_write_fn write)
{
	return persistence_writer_submit(persistence_job_kind::player, 0, 0, std::move(write));
}

struct materialization
{
	bool unavailable = false;
	bool diverge = false;
	unsigned calls = 0;
	unsigned applied = 0;
	std::vector<critical_operation_id> receipts;
	std::array<uint8_t, 2> balances{ 100, 0 };
	std::vector<std::string> order;
};

static player_save_apply_result checkpoint(const player_snapshot &snapshot, void *context)
{
	auto &state = *static_cast<materialization *>(context);
	state.order.push_back("checkpoint:" + std::to_string(snapshot.revision));
	return { player_save_apply_outcome::applied, snapshot.revision, 0 };
}

static critical_apply_result project(const critical_command &command, void *context)
{
	auto &state = *static_cast<materialization *>(context);
	++state.calls;
	if (state.unavailable)
		return { critical_apply_outcome::retryable_failure, 0, EIO };
	if (command.type == critical_command_type::player_checkpoint)
		return player_checkpoint_command_project(command, checkpoint, context);
	critical_apply_result result{ critical_apply_outcome::applied, command.outcome_revision,
				      0 };
	result.result_size = 2;
	std::copy(command.outcome_payload.begin(), command.outcome_payload.end(),
		  result.result_payload.begin());
	if (state.diverge)
	{
		++result.result_payload[1];
		return result;
	}
	for (const auto &id : state.receipts)
		if (critical_operation_id_equal(id, command.operation_id))
		{
			result.outcome = critical_apply_outcome::already_applied;
			return result;
		}
	// Model a single transactional materializer, including its idempotency receipt.
	state.receipts.push_back(command.operation_id);
	state.balances = { command.outcome_payload[0], command.outcome_payload[1] };
	state.order.push_back("transfer:" + std::to_string(state.balances[0]));
	++state.applied;
	return result;
}

static critical_command transfer(uint8_t balance)
{
	critical_command command{};
	command.schema_version = CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION;
	assert(critical_operation_id_generate(&command.operation_id));
	command.type = critical_command_type::test;
	command.payload_version = 1;
	command.source_site = critical_source_site::command;
	command.deadline_class = critical_deadline_class::interactive;
	command.accepted_at_usec = 1;
	command.keys = { { critical_entity_type::player, 1 }, { critical_entity_type::player, 2 } };
	command.payload = { balance };
	command.outcome_revision = 1;
	command.outcome_payload = { balance, static_cast<uint8_t>(100 - balance) };
	assert(critical_command_valid(command));
	return command;
}

static void pump()
{
	assert(!jobs.empty());
	const auto result = jobs.front()();
	if (result.outcome != player_save_apply_outcome::retryable_failure)
		jobs.pop_front();
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const auto first = transfer(60), second = transfer(25);
	std::vector<uint8_t> bytes;
	assert(critical_command_encode(first, &bytes) == critical_command_codec_result::ok);
	critical_command decoded{};
	assert(critical_command_decode(bytes.data(), bytes.size(), &decoded) ==
	       critical_command_codec_result::ok);
	assert(critical_command_equal(first, decoded));
	for (size_t size = 0; size < bytes.size(); ++size)
		assert(critical_command_decode(bytes.data(), size, &decoded) !=
		       critical_command_codec_result::ok);
	bytes.push_back(0);
	assert(critical_command_decode(bytes.data(), bytes.size(), &decoded) !=
	       critical_command_codec_result::ok);

	materialization state;
	assert(operation_log_init(argv[1], project, &state));
	capacity = false;
	assert(operation_log_submit(first) == critical_submit_result::overloaded);
	assert(critical_command_journal_health_copy().records == 0);
	capacity = true;
	sync_failures = 1;
	assert(operation_log_submit(first) == critical_submit_result::journal_failure);
	assert(critical_command_journal_health_copy().records == 0);
	pump();
	assert(state.calls == 0);
	state.unavailable = true;
	assert(operation_log_submit(first) == critical_submit_result::accepted);
	assert(operation_log_submit(second) == critical_submit_result::accepted);
	assert(state.calls == 0 && !operation_log_projection_caught_up());
	pump();
	assert(jobs.size() == 2 && state.balances[0] == 100);
	state.unavailable = false;
	pump();
	assert(jobs.size() == 1 && state.balances[0] == 60 && state.balances[1] == 40);
	pump();
	assert(jobs.empty() && state.balances[0] == 25 && state.balances[1] == 75);
	assert(operation_log_projection_caught_up());
	assert(operation_log_submit(second) == critical_submit_result::accepted);
	pump();
	assert(state.applied == 2);
	operation_log_shutdown();

	// Abrupt process exit after the durable ACK, before any materialization.
	const pid_t child = fork();
	assert(child >= 0);
	if (!child)
	{
		assert(operation_log_init(argv[1], project, &state));
		assert(operation_log_submit(transfer(10)) == critical_submit_result::accepted);
		_exit(0);
	}
	int status = 0;
	assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && !WEXITSTATUS(status));
	assert(operation_log_init(argv[1], project, &state));
	assert(state.balances[0] == 10 && state.balances[1] == 90 && state.applied == 3);
	assert(critical_command_journal_health_copy().records == 0);
	state.diverge = true;
	assert(operation_log_submit(transfer(5)) == critical_submit_result::accepted);
	pump();
	assert(!operation_log_ready() && jobs.size() == 1);
	assert(critical_command_journal_health_copy().records == 1);
	assert(operation_log_submit(transfer(1)) == critical_submit_result::unavailable);
	jobs.clear();
	operation_log_shutdown();
	assert(!operation_log_init(argv[1], project, &state));
	assert(critical_command_journal_health_copy().records == 1);
	operation_log_shutdown();
	state.diverge = false;
	assert(operation_log_init(argv[1], project, &state));
	assert(state.balances[0] == 5 && state.balances[1] == 95);

	// A real player checkpoint codec shares the operation ordering and supports
	// snapshots larger than the ordinary critical-command payload limit.
	player_snapshot snapshot{};
	snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	snapshot.pid = 1;
	snapshot.revision = 2;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.save_intent = 4;
	snapshot.room_vnum = 1201;
	snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
	for (size_t index = 0; index < 40; ++index)
	{
		player_item_snapshot object{};
		object.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		object.equipment_slot = -1;
		object.object_uid = index + 1;
		object.vnum = 6001;
		object.string_mask = 1;
		object.name.assign(12000, 'x');
		snapshot.items.push_back(std::move(object));
	}
	critical_command saved;
	assert(player_checkpoint_command_build(snapshot, &saved));
	assert(saved.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES);
	assert(critical_command_encode(saved, &bytes) == critical_command_codec_result::ok);
	assert(critical_command_decode(bytes.data(), bytes.size(), &decoded) ==
	       critical_command_codec_result::ok);
	assert(critical_command_equal(saved, decoded));
	state.order.clear();
	assert(operation_log_submit(transfer(4)) == critical_submit_result::accepted);
	assert(operation_log_submit_player(saved, snapshot) == critical_submit_result::accepted);
	assert(operation_log_submit(transfer(3)) == critical_submit_result::accepted);
	jobs.clear(); // Crash before the writer starts; startup replays the exact order.
	operation_log_shutdown();
	assert(operation_log_init(argv[1], project, &state));
	assert((state.order ==
		std::vector<std::string>{ "transfer:4", "checkpoint:2", "transfer:3" }));
	assert(critical_command_journal_health_copy().records == 0);
	auto invalid = saved;
	invalid.keys[0].id = 2;
	assert(player_checkpoint_command_project(invalid, checkpoint, &state).outcome ==
	       critical_apply_outcome::terminal_failure);
	operation_log_shutdown();
}
