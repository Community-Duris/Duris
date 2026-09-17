#include "player/player_death_restitution_adapter.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/config.h"
#include "player/player_load_pipeline.h"
#include "player/player_save_pipeline.h"

#include <array>
#include <limits>

extern P_desc descriptor_list;

namespace
{
constexpr size_t MAX_LIVE_SUBMISSIONS = 256;

std::array<player_death_restitution_runtime_submission, MAX_LIVE_SUBMISSIONS> live_submissions = {};

bool recipient_is_offline(uint32_t pid, void *)
{
	if (!pid || pid > static_cast<uint32_t>(std::numeric_limits<int>::max()))
		return false;
	const int target_pid = static_cast<int>(pid);

	// A descriptor that has selected this PID but has not materialized a
	// character is still an online/login claimant for fencing purposes.
	if (player_load_pipeline_pid_pending(target_pid))
		return false;
	for (P_desc descriptor = descriptor_list; descriptor; descriptor = descriptor->next)
	{
		if (descriptor->player_load_pid == target_pid)
			return false;
		const P_char candidates[] = { descriptor->character, descriptor->original };
		for (P_char character : candidates)
			if (character && IS_PC(character) && GET_PID(character) == target_pid)
				return false;
	}
	return !is_pid_online(target_pid, true);
}

bool pending_save_is_empty(uint32_t pid, uint64_t, void *)
{
	return !player_save_pipeline_target_save_pending(static_cast<int>(pid));
}

bool acquire_target_save_login_fence(uint32_t pid, uint64_t expected_revision, void *)
{
	// Recheck the world boundary immediately before reserving the save
	// barrier.  All login and gameplay mutations run on the game thread, and
	// the save pipeline reservation rejects any concurrent/new target save.
	if (!recipient_is_offline(pid, nullptr) ||
	    !player_save_pipeline_acquire_target_save_login_fence(static_cast<int>(pid),
								  expected_revision))
		return false;
	if (!recipient_is_offline(pid, nullptr))
	{
		player_save_pipeline_release_target_save_login_fence(static_cast<int>(pid),
								     expected_revision);
		return false;
	}
	return true;
}

void release_target_save_login_fence(uint32_t pid, uint64_t expected_revision, void *)
{
	player_save_pipeline_release_target_save_login_fence(static_cast<int>(pid),
							     expected_revision);
}

const player_death_restitution_runtime_callbacks live_callbacks = {
	recipient_is_offline,
	pending_save_is_empty,
	acquire_target_save_login_fence,
	release_target_save_login_fence,
};

player_death_restitution_runtime_submission *find_free_submission()
{
	for (auto &submission : live_submissions)
		if (!submission.target_save_login_fence_held)
			return &submission;
	return nullptr;
}

player_death_restitution_runtime_submission *
find_submission(const critical_operation_id &operation_id)
{
	for (auto &submission : live_submissions)
		if (submission.target_save_login_fence_held &&
		    critical_operation_id_equal(submission.operation_id, operation_id))
			return &submission;
	return nullptr;
}
} // namespace

player_death_restitution_runtime_result player_death_restitution_runtime_submit_live(
	const player_death_restitution_plan &plan,
	player_death_restitution_runtime_submission *submission_out)
{
	if (submission_out)
		*submission_out = {};
	player_death_restitution_runtime_submission *slot = find_free_submission();
	if (!slot)
		return player_death_restitution_runtime_result::overloaded;

	player_death_restitution_runtime_submission submission = {};
	const player_death_restitution_runtime_result result =
		player_death_restitution_runtime_submit(plan, live_callbacks, nullptr, &submission);
	if (result == player_death_restitution_runtime_result::accepted ||
	    result == player_death_restitution_runtime_result::awaiting_durability ||
	    result == player_death_restitution_runtime_result::attached ||
	    result == player_death_restitution_runtime_result::journal_uncertain)
	{
		*slot = submission;
		if (submission_out)
			*submission_out = submission;
	}
	return result;
}

player_death_restitution_runtime_result player_death_restitution_runtime_submit_live_approved(
	const critical_command &approved_command, const char *actor, int actor_level,
	player_death_restitution_runtime_submission *submission_out)
{
	if (submission_out)
		*submission_out = {};
	if (!actor || !*actor || actor_level < FORGER)
		return player_death_restitution_runtime_result::unauthorized;
	if (!critical_command_valid(approved_command) ||
	    approved_command.type != critical_command_type::player_death_restitution ||
	    approved_command.source_site != critical_source_site::operator_repair ||
	    approved_command.deadline_class != critical_deadline_class::interactive)
		return player_death_restitution_runtime_result::invalid_plan;

	player_death_restitution_plan plan = {};
	if (!player_death_restitution_command_decode_payload(approved_command, &plan))
		return player_death_restitution_runtime_result::invalid_plan;
	if (plan.actor != actor)
		return player_death_restitution_runtime_result::identity_conflict;

	player_death_restitution_runtime_submission *slot = find_free_submission();
	if (!slot)
		return player_death_restitution_runtime_result::overloaded;
	player_death_restitution_runtime_submission submission = {};
	const player_death_restitution_runtime_result result =
		player_death_restitution_runtime_submit_command(approved_command, live_callbacks,
								nullptr, &submission);
	if (result == player_death_restitution_runtime_result::accepted ||
	    result == player_death_restitution_runtime_result::awaiting_durability ||
	    result == player_death_restitution_runtime_result::attached ||
	    result == player_death_restitution_runtime_result::journal_uncertain)
	{
		*slot = submission;
		if (submission_out)
			*submission_out = submission;
	}
	return result;
}

bool player_death_restitution_runtime_restore_replayed_command(const critical_command &command,
							       void *)
{
	if (command.type != critical_command_type::player_death_restitution)
		return true;
	player_death_restitution_runtime_submission *slot = find_free_submission();
	if (!slot)
		return false;
	player_death_restitution_runtime_submission submission = {};
	if (player_death_restitution_runtime_restore_replayed_command(command, live_callbacks,
								      nullptr, &submission) !=
	    player_death_restitution_runtime_result::accepted)
		return false;
	*slot = submission;
	return true;
}

void player_death_restitution_runtime_handle_completions(const critical_completion *completions,
							 size_t count)
{
	if (!completions)
		return;
	for (size_t index = 0; index < count; ++index)
	{
		player_death_restitution_runtime_submission *submission =
			find_submission(completions[index].operation_id);
		if (submission)
			(void)player_death_restitution_runtime_complete(
				submission, completions[index], live_callbacks, nullptr);
	}
}

void player_death_restitution_runtime_abort_all(void)
{
	for (auto &submission : live_submissions)
		if (submission.target_save_login_fence_held)
			player_death_restitution_runtime_abort(&submission, live_callbacks,
							       nullptr);
}

void player_death_restitution_runtime_shutdown(void)
{
	// Do not release accepted submissions here.  A normal graceful drain has
	// already delivered conclusive completions; if the coordinator is
	// ambiguous, retaining this target fence is what makes the journal replay
	// safe on the next process.  Pipeline teardown follows this call and is the
	// final process-lifetime cleanup.  Failed initialization uses abort_all().
}

bool player_death_restitution_runtime_login_admit(int pid)
{
	return pid > 0 && player_save_pipeline_save_admitted(pid);
}

player_death_restitution_runtime_live_health player_death_restitution_runtime_live_health_copy(void)
{
	player_death_restitution_runtime_live_health health = {};
	for (const auto &submission : live_submissions)
		if (submission.target_save_login_fence_held)
		{
			++health.pending_operations;
			++health.fenced_targets;
		}
	return health;
}
