#include "persistence/operation_log.h"
#include "player/player_save_worker.h"
#include "player/player_checkpoint_command.h"

#include <cassert>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <new>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>

static std::deque<persistence_job_write_fn> jobs;
static bool capacity = true;
static int sync_failures = 0;
static int sync_failure_at = 0;
static std::atomic<bool> hold_projection{ false }, projection_entered{ false };
static unsigned legacy_calls = 0;
static bool legacy_rejection = false;
static bool fail_receipt_allocation = false;
static bool fail_next_allocation = false;
extern "C" void *__real__Znwm(size_t);
extern "C" void *__wrap__Znwm(size_t size)
{
	if (fail_next_allocation)
	{
		fail_next_allocation = false;
		throw std::bad_alloc();
	}
	return __real__Znwm(size);
}
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd)
{
	if (sync_failures || (sync_failure_at && !--sync_failure_at))
	{
		if (sync_failures)
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
	bool diverge_revision = false;
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
	if (command.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION)
	{
		++legacy_calls;
		if (legacy_rejection)
			return { critical_apply_outcome::terminal_failure, 0, EACCES };
		auto reconciled = command;
		reconciled.schema_version = CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION;
		reconciled.outcome_revision = 1;
		reconciled.outcome_payload = { command.payload[0],
					       static_cast<uint8_t>(100 - command.payload[0]) };
		return project(reconciled, context);
	}
	projection_entered = true;
	while (hold_projection)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
	if (state.diverge_revision)
	{
		++result.durable_revision;
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
	if (fail_receipt_allocation)
	{
		fail_receipt_allocation = false;
		fail_next_allocation = true;
	}
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
	for (auto outcome :
	     { critical_apply_outcome::applied, critical_apply_outcome::already_applied })
	{
		critical_apply_result result{ outcome, first.outcome_revision, 0 };
		result.result_size = first.outcome_payload.size();
		std::copy(first.outcome_payload.begin(), first.outcome_payload.end(),
			  result.result_payload.begin());
		assert(operation_log_result_matches(first, result));
		result.durable_revision = first.outcome_revision - 1;
		assert(!operation_log_result_matches(first, result));
		result.durable_revision = first.outcome_revision + 1;
		assert(!operation_log_result_matches(first, result));
	}
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
	assert(operation_log_submit(first) == critical_submit_result::attached);
	auto conflicting = first;
	conflicting.outcome_payload[0] = 59;
	assert(operation_log_submit(conflicting) == critical_submit_result::identity_conflict);
	assert(jobs.size() == 1);
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
	assert(operation_log_submit(second) == critical_submit_result::attached);
	assert(jobs.empty());
	assert(state.applied == 2);
	operation_log_shutdown();
	assert(operation_log_init(argv[1], project, &state));
	critical_apply_result receipt{};
	auto duplicate = second;
	duplicate.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	duplicate.outcome_revision = 0;
	duplicate.outcome_payload.clear();
	duplicate.accepted_at_usec = 100; // A retry's arrival time is not request identity.
	assert(operation_log_lookup(duplicate, &receipt) == critical_receipt_status::found);
	assert(receipt.durable_revision == second.outcome_revision && receipt.result_size == 2);
	assert(receipt.result_payload[0] == 25);
	assert(operation_log_submit(second) == critical_submit_result::attached);
	duplicate.payload[0] = 24;
	assert(operation_log_lookup(duplicate, &receipt) ==
	       critical_receipt_status::identity_conflict);
	assert(state.calls == 3 && jobs.empty());
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
	auto obsolete_job = std::move(jobs.front());
	jobs.clear();
	operation_log_shutdown();
	assert(!operation_log_init(argv[1], project, &state));
	assert(critical_command_journal_health_copy().records == 1);
	operation_log_shutdown();
	state.diverge = false;
	assert(operation_log_init(argv[1], project, &state));
	assert(state.balances[0] == 5 && state.balances[1] == 95);
	const auto calls_before_old_job = state.calls;
	assert(obsolete_job().outcome == player_save_apply_outcome::retryable_failure);
	assert(state.calls == calls_before_old_job && operation_log_projection_caught_up());

	// Matching bytes at a different revision are not the promised outcome.
	// Keep the redo record and fence later writes, including on cold replay.
	state.diverge_revision = true;
	assert(operation_log_submit(transfer(4)) == critical_submit_result::accepted);
	pump();
	assert(!operation_log_ready() && jobs.size() == 1);
	assert(critical_command_journal_health_copy().records == 1);
	assert(operation_log_submit(transfer(1)) == critical_submit_result::unavailable);
	jobs.clear();
	operation_log_shutdown();
	assert(!operation_log_init(argv[1], project, &state));
	assert(critical_command_journal_health_copy().records == 1);
	operation_log_shutdown();
	state.diverge_revision = false;
	assert(operation_log_init(argv[1], project, &state));
	assert(state.balances[0] == 4 && state.balances[1] == 96);
	assert(critical_command_journal_health_copy().records == 0);

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

	// Failure to persist the receipt cannot discard the redo record. Projection
	// retries must use the materializer's existing idempotency receipt.
	const auto faulted = transfer(2);
	assert(operation_log_submit(faulted) == critical_submit_result::accepted);
	const auto applied_before_failure = state.applied;
	sync_failures = 1;
	pump();
	assert(jobs.size() == 1 && state.applied == applied_before_failure + 1);
	assert(critical_command_journal_health_copy().records == 1);
	pump();
	assert(jobs.empty() && state.applied == applied_before_failure + 1);

	// Receipt construction can allocate after the backend commits. Its failure
	// must remain retryable: the ordered writer must not drop this logged job.
	const auto allocation_fault = transfer(11);
	assert(operation_log_submit(allocation_fault) == critical_submit_result::accepted);
	const auto applied_before_allocation = state.applied;
	fail_receipt_allocation = true;
	pump();
	assert(!fail_next_allocation && jobs.size() == 1);
	assert(state.applied == applied_before_allocation + 1);
	assert(critical_command_journal_health_copy().records == 1);
	assert(!operation_log_projection_caught_up());
	pump();
	assert(jobs.empty() && state.applied == applied_before_allocation + 1);
	assert(operation_log_projection_caught_up());

	// The receipt survives a crash between its directory sync and WAL checkpoint.
	const auto checkpoint_fault = transfer(1);
	assert(operation_log_submit(checkpoint_fault) == critical_submit_result::accepted);
	sync_failure_at = 3;
	pump();
	assert(jobs.size() == 1 && critical_command_journal_health_copy().records == 1);
	jobs.clear();
	operation_log_shutdown();
	const auto applied_before_replay = state.applied;
	assert(operation_log_init(argv[1], project, &state));
	assert(state.applied == applied_before_replay && operation_log_projection_caught_up());
	assert(operation_log_submit(checkpoint_fault) == critical_submit_result::attached);

	// Shutdown waits for an active projector before releasing its context/lease.
	assert(operation_log_submit(transfer(0)) == critical_submit_result::accepted);
	hold_projection = true;
	projection_entered = false;
	std::thread writer([] { pump(); });
	while (!projection_entered)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	std::atomic<bool> shutdown_done{ false };
	std::thread shutdown(
		[&]
		{
			operation_log_shutdown();
			shutdown_done = true;
		});
	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	assert(!shutdown_done);
	hold_projection = false;
	writer.join();
	shutdown.join();
	assert(shutdown_done && jobs.empty());
	assert(operation_log_init(argv[1], project, &state));
	operation_log_shutdown();

	// Adopt an older journal without changing the identity sent to its backend.
	assert(operation_log_init(argv[1], project, &state));
	auto policy_rejection = transfer(48);
	policy_rejection.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	policy_rejection.outcome_revision = 0;
	policy_rejection.outcome_payload.clear();
	const critical_apply_result rejected{ critical_apply_outcome::terminal_failure, 0, ENOSPC };
	assert(operation_log_remember_rejection(policy_rejection, rejected));
	assert(operation_log_remember_rejection(policy_rejection, rejected));
	operation_log_shutdown();
	assert(operation_log_init(argv[1], project, &state));
	assert(operation_log_lookup(policy_rejection, &receipt) == critical_receipt_status::found);
	assert(receipt.outcome == critical_apply_outcome::terminal_failure &&
	       receipt.error_code == ENOSPC);
	assert(!operation_log_remember_rejection(
		policy_rejection, { critical_apply_outcome::terminal_failure, 0, EACCES }));
	operation_log_shutdown();

	// Adopt an older journal without changing the identity sent to its backend.
	// A blocked reconciliation retains the legacy record, then the successful
	// recovery keeps a receipt after that record is checkpointed away.
	auto legacy = transfer(50);
	legacy.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	legacy.outcome_revision = 0;
	legacy.outcome_payload.clear();
	assert(critical_command_journal_init(argv[1]));
	assert(critical_command_journal_append(legacy) == critical_command_journal_result::ok);
	critical_command_journal_shutdown();
	state.unavailable = true;
	assert(!operation_log_init(argv[1], project, &state));
	assert(critical_command_journal_health_copy().records == 1);
	operation_log_shutdown();
	state.unavailable = false;
	assert(operation_log_init(argv[1], project, &state));
	assert(legacy_calls == 2 && state.balances[0] == 50);
	assert(critical_command_journal_health_copy().records == 0);
	assert(operation_log_lookup(legacy, &receipt) == critical_receipt_status::found);
	assert(receipt.result_payload[0] == 50 && receipt.durable_revision == 1);

	// A conclusive old policy rejection must not block every future boot, or
	// become a fresh request after its original WAL record is checkpointed.
	operation_log_shutdown();
	auto refused = transfer(49);
	refused.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	refused.outcome_revision = 0;
	refused.outcome_payload.clear();
	assert(critical_command_journal_init(argv[1]));
	assert(critical_command_journal_append(refused) == critical_command_journal_result::ok);
	critical_command_journal_shutdown();
	legacy_rejection = true;
	assert(operation_log_init(argv[1], project, &state));
	assert(critical_command_journal_health_copy().records == 0);
	assert(operation_log_lookup(refused, &receipt) == critical_receipt_status::found);
	assert(receipt.outcome == critical_apply_outcome::terminal_failure &&
	       receipt.error_code == EACCES && !receipt.result_size);
	operation_log_shutdown();
	legacy_rejection = false;
	assert(operation_log_init(argv[1], project, &state));
	assert(operation_log_lookup(refused, &receipt) == critical_receipt_status::found &&
	       receipt.error_code == EACCES);

	// A damaged durable receipt must not be mistaken for a new request.
	char receipt_name[CRITICAL_COMMAND_ID_HEX_SIZE]{};
	assert(critical_operation_id_to_hex(faulted.operation_id, receipt_name,
					    sizeof(receipt_name)));
	const auto receipt_path = std::filesystem::path(argv[1]) / "receipts" / receipt_name;
	std::fstream damaged(receipt_path, std::ios::in | std::ios::out | std::ios::binary);
	assert(damaged);
	damaged.seekp(4);
	damaged.put('x');
	damaged.close();
	assert(operation_log_lookup(faulted, &receipt) == critical_receipt_status::unavailable);
	assert(!operation_log_ready());
	assert(operation_log_submit(transfer(1)) == critical_submit_result::unavailable);
	operation_log_shutdown();
}
