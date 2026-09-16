#include "core/prototypes.h"
#include "player/player_death_restitution_adapter.h"
#include "player/player_load_pipeline.h"
#include "player/player_save_pipeline.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <openssl/sha.h>

P_desc descriptor_list = nullptr;

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}

namespace
{
constexpr int RECIPIENT_PID = 20;

std::atomic<bool> crash_phase = false;
std::atomic<unsigned int> restart_apply_calls = 0;
std::mutex apply_mutex;
std::condition_variable apply_changed;
bool apply_started = false;
bool allow_crash_apply = false;
bool allow_second_restart_apply = false;
bool target_fence_held = false;
int release_calls = 0;

critical_operation_id operation_id(uint8_t seed)
{
	critical_operation_id value = {};
	for (size_t index = 0; index < value.bytes.size(); ++index)
		value.bytes[index] = static_cast<uint8_t>(seed + index);
	return value;
}

player_death_restitution_plan valid_plan()
{
	player_death_restitution_item_state state = {};
	state.item_uid = 101;
	state.vnum = 7101;
	state.quantity = 1;
	state.weight = 4;
	state.cost = 99;
	state.timer = -1;
	state.item_type = 1;
	state.material = 2;
	state.condition = 100;
	std::vector<uint8_t> encoded_state;
	assert(player_death_restitution_item_state_encode(state, &encoded_state));

	player_death_restitution_item item = {};
	item.item_uid = state.item_uid;
	item.source_root_item_uid = state.item_uid;
	item.delivered_root_item_uid = state.item_uid;
	item.source_item_revision = 12;
	item.custody_item_revision = 9;
	item.expected_item_revision = 12;
	item.expected_owner_revision = 4;
	item.expected_owner_state = PLAYER_DEATH_RESTITUTION_QUARANTINED_STATE;
	item.custody_state = 1;
	item.custody_owner_type = PLAYER_DEATH_RESTITUTION_PLAYER_OWNER_TYPE;
	item.custody_owner_id = 10;
	item.vnum = state.vnum;
	item.disposition = player_death_restitution_disposition::deliver;
	item.classification = "ordinary_item";
	item.note = "restart harness";
	item.metadata_payload = encoded_state;
	item.original_payload = { 0xaa };
	SHA256(encoded_state.data(), encoded_state.size(), item.metadata_digest.data());

	player_death_restitution_plan plan = {};
	plan.source_pid = 10;
	plan.death_revision = 77;
	plan.recipient_pid = RECIPIENT_PID;
	plan.restitution_id = operation_id(1);
	plan.death_operation_id = operation_id(33);
	plan.evidence_digest.fill(0x11);
	plan.payload_digest.fill(0x33);
	plan.plan_digest.fill(0x22);
	plan.expected_recipient_save_revision = 8;
	plan.expected_source_owner_revision = 4;
	plan.expected_recipient_owner_revision = 6;
	plan.loss_epoch = 1700000000;
	plan.actor = "restart-harness";
	plan.reason = "journal crash restart";
	plan.items.push_back(item);
	assert(player_death_restitution_plan_valid(plan));
	return plan;
}

critical_apply_result apply(const critical_command &command, void *)
{
	assert(command.type == critical_command_type::player_death_restitution);
	if (crash_phase.load())
	{
		std::unique_lock<std::mutex> lock(apply_mutex);
		apply_started = true;
		apply_changed.notify_all();
		apply_changed.wait(lock, [] { return allow_crash_apply; });
		return { critical_apply_outcome::ambiguous_commit, 0, 777 };
	}

	const unsigned int call = ++restart_apply_calls;
	if (call == 1)
		return { critical_apply_outcome::ambiguous_commit, 0, 777 };
	std::unique_lock<std::mutex> lock(apply_mutex);
	apply_changed.wait(lock, [] { return allow_second_restart_apply; });
	return { critical_apply_outcome::applied, 1, 0 };
}

} // namespace

bool is_pid_online(int pid, bool)
{
	assert(pid == RECIPIENT_PID);
	return false;
}

bool player_load_pipeline_pid_pending(int pid)
{
	assert(pid == RECIPIENT_PID);
	return false;
}

bool player_save_pipeline_target_save_pending(int pid)
{
	assert(pid == RECIPIENT_PID);
	return false;
}

bool player_save_pipeline_acquire_target_save_login_fence(int pid, player_revision_t revision)
{
	assert(pid == RECIPIENT_PID && revision == 8);
	if (target_fence_held)
		return false;
	target_fence_held = true;
	return true;
}

void player_save_pipeline_release_target_save_login_fence(int pid, player_revision_t revision)
{
	assert(pid == RECIPIENT_PID && revision == 8);
	++release_calls;
	target_fence_held = false;
}

bool player_save_pipeline_save_admitted(int pid)
{
	assert(pid == RECIPIENT_PID || pid == RECIPIENT_PID + 1);
	return pid != RECIPIENT_PID || !target_fence_held;
}

namespace
{
	template <typename Predicate> void wait_until(Predicate predicate)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (!predicate())
	{
		assert(std::chrono::steady_clock::now() < deadline);
		std::this_thread::yield();
	}
}

void run_crash_stage(const std::string &directory)
{
	crash_phase = true;
	assert(critical_command_coordinator_init(
		directory.c_str(), apply, nullptr, 1,
		player_death_restitution_runtime_restore_replayed_command, nullptr));
	player_death_restitution_runtime_submission submission = {};
	assert(player_death_restitution_runtime_submit_live(valid_plan(), &submission) ==
	       player_death_restitution_runtime_result::accepted);
	assert(target_fence_held && !player_death_restitution_runtime_login_admit(RECIPIENT_PID));
	assert(player_death_restitution_runtime_login_admit(RECIPIENT_PID + 1));

	{
		std::unique_lock<std::mutex> lock(apply_mutex);
		apply_changed.wait_for(lock, std::chrono::seconds(1), [] { return apply_started; });
	}
	// The command append is fsync'd before submit_live returns.  Exit without
	// coordinator/runtime shutdown to model a process crash with the journal
	// record still present and the in-memory fence gone with the process.
	::_exit(0);
}

void run_restart_stage(const std::string &directory)
{
	crash_phase = false;
	assert(critical_command_coordinator_init(
		directory.c_str(), apply, nullptr, 1,
		player_death_restitution_runtime_restore_replayed_command, nullptr));
	// The replay hook runs inside coordinator init, before this process can
	// accept a login or publish a worker completion.
	assert(target_fence_held);
	assert(!player_death_restitution_runtime_login_admit(RECIPIENT_PID));
	assert(player_death_restitution_runtime_login_admit(RECIPIENT_PID + 1));
	assert(critical_command_coordinator_health_copy().fenced_keys == 1);

	critical_completion completions[8] = {};
	wait_until([&]
	{
		const size_t count = critical_command_coordinator_pulse(completions, 8);
		player_death_restitution_runtime_handle_completions(completions, count);
		return critical_command_coordinator_health_copy().retries == 1;
	});
	// An ambiguous commit is retried, not treated as terminal: the target
	// pipeline fence remains held while the database outcome is unknown.
	assert(target_fence_held);
	assert(!player_death_restitution_runtime_login_admit(RECIPIENT_PID));

	{
		std::lock_guard<std::mutex> lock(apply_mutex);
		allow_second_restart_apply = true;
	}
	apply_changed.notify_all();
	wait_until([&]
	{
		const size_t count = critical_command_coordinator_pulse(completions, 8);
		player_death_restitution_runtime_handle_completions(completions, count);
		return critical_command_coordinator_health_copy().completed == 1;
	});
	assert(!target_fence_held);
	assert(player_death_restitution_runtime_login_admit(RECIPIENT_PID));
	assert(release_calls == 1);
	assert(critical_command_journal_health_copy().records == 0);
	critical_command_coordinator_shutdown();
}
} // namespace

int main(int argc, char **argv)
{
	assert(argc == 3);
	const std::string stage = argv[1];
	if (stage == "crash")
		run_crash_stage(argv[2]);
	else if (stage == "restart")
		run_restart_stage(argv[2]);
	else
		return 2;
	return 0;
}
