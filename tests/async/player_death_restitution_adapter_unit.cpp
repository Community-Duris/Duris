#include "core/prototypes.h"
#include "player/player_death_restitution_adapter.h"
#include "player/player_load_pipeline.h"
#include "player/player_save_pipeline.h"

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <openssl/sha.h>

P_desc descriptor_list = nullptr;

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}

namespace
{
bool target_online = false;
bool target_load_pending = false;
bool target_save_pending = false;
bool target_fence_held = false;
int acquire_calls = 0;
int release_calls = 0;
critical_submit_result coordinator_result = critical_submit_result::journal_uncertain;
critical_operation_id submitted_operation = {};

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
	item.note = "adapter test";
	item.metadata_payload = encoded_state;
	item.original_payload = { 0xaa };
	SHA256(encoded_state.data(), encoded_state.size(), item.metadata_digest.data());

	player_death_restitution_plan plan = {};
	plan.source_pid = 10;
	plan.death_revision = 77;
	plan.recipient_pid = 20;
	plan.restitution_id = operation_id(1);
	plan.death_operation_id = operation_id(33);
	plan.evidence_digest.fill(0x11);
	plan.payload_digest.fill(0x33);
	plan.plan_digest.fill(0x22);
	plan.expected_recipient_save_revision = 8;
	plan.expected_source_owner_revision = 4;
	plan.expected_recipient_owner_revision = 6;
	plan.loss_epoch = 1700000000;
	plan.actor = "adapter-test";
	plan.reason = "death restitution";
	plan.items.push_back(item);
	assert(player_death_restitution_plan_valid(plan));
	return plan;
}
} // namespace

bool is_pid_online(int pid, bool)
{
	assert(pid == 20);
	return target_online;
}

bool player_load_pipeline_pid_pending(int pid)
{
	assert(pid == 20);
	return target_load_pending;
}

bool player_save_pipeline_target_save_pending(int pid)
{
	assert(pid == 20);
	return target_save_pending;
}

bool player_save_pipeline_acquire_target_save_login_fence(int pid, player_revision_t revision)
{
	assert(pid == 20);
	assert(revision == 8);
	++acquire_calls;
	if (target_fence_held)
		return false;
	target_fence_held = true;
	return true;
}

void player_save_pipeline_release_target_save_login_fence(int pid, player_revision_t revision)
{
	assert(pid == 20);
	assert(revision == 8);
	++release_calls;
	target_fence_held = false;
}

bool player_save_pipeline_save_admitted(int pid)
{
	assert(pid == 20 || pid == 21);
	return pid != 20 || !target_fence_held;
}

critical_submit_result critical_command_coordinator_submit(critical_command command)
{
	submitted_operation = command.operation_id;
	return coordinator_result;
}

int main()
{
	const player_death_restitution_plan plan = valid_plan();
	player_death_restitution_runtime_submission submission = {};

	target_online = true;
	assert(player_death_restitution_runtime_submit_live(plan, &submission) ==
	       player_death_restitution_runtime_result::recipient_online);
	target_online = false;

	target_load_pending = true;
	assert(player_death_restitution_runtime_submit_live(plan, &submission) ==
	       player_death_restitution_runtime_result::recipient_online);
	target_load_pending = false;

	target_save_pending = true;
	assert(player_death_restitution_runtime_submit_live(plan, &submission) ==
	       player_death_restitution_runtime_result::pending_save);
	target_save_pending = false;

	assert(player_death_restitution_runtime_submit_live(plan, &submission) ==
	       player_death_restitution_runtime_result::journal_uncertain);
	assert(submission.target_save_login_fence_held);
	assert(target_fence_held);
	assert(!player_death_restitution_runtime_login_admit(20));
	assert(player_death_restitution_runtime_login_admit(21));
	const auto held_health = player_death_restitution_runtime_live_health_copy();
	assert(held_health.pending_operations == 1 && held_health.fenced_targets == 1);

	critical_completion completion = {};
	completion.operation_id = submitted_operation;
	completion.outcome = critical_apply_outcome::ambiguous_commit;
	player_death_restitution_runtime_handle_completions(&completion, 1);
	assert(target_fence_held);
	assert(!player_death_restitution_runtime_login_admit(20));
	assert(release_calls == 0);
	player_death_restitution_runtime_shutdown();
	assert(target_fence_held);
	assert(!player_death_restitution_runtime_login_admit(20));
	assert(release_calls == 0);
	const auto ambiguous_health = player_death_restitution_runtime_live_health_copy();
	assert(ambiguous_health.pending_operations == 1 && ambiguous_health.fenced_targets == 1);

	// Only a later conclusive completion releases the per-target fence.
	completion.outcome = critical_apply_outcome::applied;
	player_death_restitution_runtime_handle_completions(&completion, 1);
	assert(!target_fence_held);
	assert(player_death_restitution_runtime_login_admit(20));
	assert(release_calls == 1);
	const auto clear_health = player_death_restitution_runtime_live_health_copy();
	assert(clear_health.pending_operations == 0 && clear_health.fenced_targets == 0);

	assert(acquire_calls == 1);
	return 0;
}
