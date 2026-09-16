#include "player/player_death_restitution_runtime.h"

namespace
{
bool callbacks_complete(const player_death_restitution_runtime_callbacks &callbacks)
{
	return callbacks.recipient_is_offline && callbacks.pending_save_is_empty &&
	       callbacks.acquire_target_save_login_fence &&
	       callbacks.release_target_save_login_fence;
}

player_death_restitution_runtime_result map_submit_result(critical_submit_result result)
{
	switch (result)
	{
	case critical_submit_result::accepted:
		return player_death_restitution_runtime_result::accepted;
	case critical_submit_result::attached:
		return player_death_restitution_runtime_result::attached;
	case critical_submit_result::invalid:
		return player_death_restitution_runtime_result::invalid_plan;
	case critical_submit_result::unavailable:
		return player_death_restitution_runtime_result::coordinator_unavailable;
	case critical_submit_result::overloaded:
		return player_death_restitution_runtime_result::overloaded;
	case critical_submit_result::journal_uncertain:
		return player_death_restitution_runtime_result::journal_uncertain;
	case critical_submit_result::journal_failure:
		return player_death_restitution_runtime_result::journal_failure;
	case critical_submit_result::identity_conflict:
		return player_death_restitution_runtime_result::identity_conflict;
	}
	return player_death_restitution_runtime_result::journal_failure;
}

void clear_submission(player_death_restitution_runtime_submission *submission)
{
	if (!submission)
		return;
	submission->operation_id = {};
	submission->recipient_pid = 0;
	submission->expected_save_revision = 0;
	submission->target_save_login_fence_held = false;
}
}

player_death_restitution_runtime_result player_death_restitution_runtime_preflight(
	const player_death_restitution_plan &plan,
	const player_death_restitution_runtime_callbacks &callbacks, void *context)
{
	if (!player_death_restitution_plan_valid(plan) || !callbacks_complete(callbacks))
		return player_death_restitution_runtime_result::invalid_plan;
	if (!callbacks.recipient_is_offline(plan.recipient_pid, context))
		return player_death_restitution_runtime_result::recipient_online;
	if (!callbacks.pending_save_is_empty(plan.recipient_pid,
					     plan.expected_recipient_save_revision, context))
		return player_death_restitution_runtime_result::pending_save;
	return player_death_restitution_runtime_result::accepted;
}

namespace
{
player_death_restitution_runtime_result
submit_command_internal(const critical_command &command, const player_death_restitution_plan &plan,
			const player_death_restitution_runtime_callbacks &callbacks, void *context,
			player_death_restitution_runtime_submission *submission)
{
	const player_death_restitution_runtime_result preflight =
		player_death_restitution_runtime_preflight(plan, callbacks, context);
	if (preflight != player_death_restitution_runtime_result::accepted)
		return preflight;
	if (!callbacks.acquire_target_save_login_fence(
		    plan.recipient_pid, plan.expected_recipient_save_revision, context))
		return player_death_restitution_runtime_result::fence_unavailable;
	const critical_submit_result submitted = critical_command_coordinator_submit(command);
	const player_death_restitution_runtime_result mapped = map_submit_result(submitted);
	if (!critical_submit_result_keeps_operation(submitted))
	{
		callbacks.release_target_save_login_fence(
			plan.recipient_pid, plan.expected_recipient_save_revision, context);
		return mapped;
	}
	submission->operation_id = command.operation_id;
	submission->recipient_pid = plan.recipient_pid;
	submission->expected_save_revision = plan.expected_recipient_save_revision;
	submission->target_save_login_fence_held = true;
	return mapped;
}
} // namespace

player_death_restitution_runtime_result player_death_restitution_runtime_submit_command(
	const critical_command &command,
	const player_death_restitution_runtime_callbacks &callbacks, void *context,
	player_death_restitution_runtime_submission *submission)
{
	if (!submission)
		return player_death_restitution_runtime_result::invalid_plan;
	clear_submission(submission);
	if (!critical_command_valid(command) ||
	    command.type != critical_command_type::player_death_restitution)
		return player_death_restitution_runtime_result::invalid_plan;
	player_death_restitution_plan plan = {};
	if (!player_death_restitution_command_decode_payload(command, &plan))
		return player_death_restitution_runtime_result::invalid_plan;
	return submit_command_internal(command, plan, callbacks, context, submission);
}

player_death_restitution_runtime_result
player_death_restitution_runtime_restore_replayed_command(
	const critical_command &command,
	const player_death_restitution_runtime_callbacks &callbacks, void *context,
	player_death_restitution_runtime_submission *submission)
{
	if (!submission)
		return player_death_restitution_runtime_result::invalid_plan;
	clear_submission(submission);
	if (!critical_command_valid(command) ||
	    command.type != critical_command_type::player_death_restitution)
		return player_death_restitution_runtime_result::invalid_plan;
	player_death_restitution_plan plan = {};
	if (!player_death_restitution_command_decode_payload(command, &plan))
		return player_death_restitution_runtime_result::invalid_plan;
	const player_death_restitution_runtime_result preflight =
		player_death_restitution_runtime_preflight(plan, callbacks, context);
	if (preflight != player_death_restitution_runtime_result::accepted)
		return preflight;
	if (!callbacks.acquire_target_save_login_fence(
		    plan.recipient_pid, plan.expected_recipient_save_revision, context))
		return player_death_restitution_runtime_result::fence_unavailable;
	submission->operation_id = command.operation_id;
	submission->recipient_pid = plan.recipient_pid;
	submission->expected_save_revision = plan.expected_recipient_save_revision;
	submission->target_save_login_fence_held = true;
	return player_death_restitution_runtime_result::accepted;
}

player_death_restitution_runtime_result
player_death_restitution_runtime_submit(const player_death_restitution_plan &plan,
					const player_death_restitution_runtime_callbacks &callbacks,
					void *context,
					player_death_restitution_runtime_submission *submission)
{
	if (!submission)
		return player_death_restitution_runtime_result::invalid_plan;
	clear_submission(submission);
	critical_command command = {};
	if (!player_death_restitution_command_build(plan, &command))
		return player_death_restitution_runtime_result::invalid_plan;
	return submit_command_internal(command, plan, callbacks, context, submission);
}

void player_death_restitution_runtime_abort(
	player_death_restitution_runtime_submission *submission,
	const player_death_restitution_runtime_callbacks &callbacks, void *context)
{
	if (!submission || !submission->target_save_login_fence_held)
		return;
	if (callbacks.release_target_save_login_fence)
		callbacks.release_target_save_login_fence(
			submission->recipient_pid, submission->expected_save_revision, context);
	clear_submission(submission);
}

bool player_death_restitution_runtime_complete(
	player_death_restitution_runtime_submission *submission,
	const critical_completion &completion,
	const player_death_restitution_runtime_callbacks &callbacks, void *context)
{
	if (!submission || !submission->target_save_login_fence_held ||
	    !critical_operation_id_equal(submission->operation_id, completion.operation_id))
		return false;
	const bool terminal = completion.outcome == critical_apply_outcome::applied ||
			      completion.outcome == critical_apply_outcome::already_applied ||
			      completion.outcome == critical_apply_outcome::terminal_failure;
	// Retryable and ambiguous outcomes are deliberately not terminal.  Keep
	// the recipient fence held until the coordinator publishes a conclusive
	// completion; releasing it here could admit a login while the database
	// outcome is still unknown.
	if (!terminal)
		return false;
	player_death_restitution_runtime_abort(submission, callbacks, context);
	return true;
}
