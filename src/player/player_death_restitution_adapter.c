#include "player/player_death_restitution_adapter.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/config.h"
#include "player/player_load_pipeline.h"
#include "player/player_save_pipeline.h"

#include <array>
#include <cstring>
#include <limits>
#include <string>

extern P_desc descriptor_list;

namespace
{
constexpr size_t MAX_LIVE_SUBMISSIONS = 256;
constexpr size_t MAX_OPERATION_STATUS_RECORDS = MAX_LIVE_SUBMISSIONS * 2;

std::array<player_death_restitution_runtime_submission, MAX_LIVE_SUBMISSIONS> live_submissions = {};

struct operation_status_record
{
	bool in_use = false;
	critical_operation_id operation_id = {};
	std::array<uint8_t, 32> plan_digest = {};
	uint32_t schema_version = 0;
	uint16_t payload_version = 0;
	critical_source_site source_site = critical_source_site::unknown;
	critical_deadline_class deadline_class = critical_deadline_class::interactive;
	uint64_t accepted_at_usec = 0;
	std::string actor;
	player_death_restitution_runtime_operation_status status = {};
};

// Status records survive descriptor disconnects but are bounded and contain
// only the protected actor binding plus phase metadata.  They are not a
// replacement for the durable verification tool after a process restart.
std::array<operation_status_record, MAX_OPERATION_STATUS_RECORDS> operation_statuses = {};

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

operation_status_record *find_status(const critical_operation_id &operation_id)
{
	if (critical_operation_id_is_zero(operation_id))
		return nullptr;
	for (auto &record : operation_statuses)
		if (record.in_use && critical_operation_id_equal(record.operation_id, operation_id))
			return &record;
	return nullptr;
}

operation_status_record *reserve_status(const critical_command &command,
					const player_death_restitution_plan &plan,
					const char *actor)
{
	if (!actor || !*actor || critical_operation_id_is_zero(command.operation_id))
		return nullptr;
	if (operation_status_record *existing = find_status(command.operation_id))
		return existing;
	for (auto &record : operation_statuses)
	{
		if (record.in_use)
			continue;
		try
		{
			record.in_use = true;
			record.operation_id = command.operation_id;
			record.plan_digest = plan.plan_digest;
			record.schema_version = command.schema_version;
			record.payload_version = command.payload_version;
			record.source_site = command.source_site;
			record.deadline_class = command.deadline_class;
			record.accepted_at_usec = command.accepted_at_usec;
			record.actor = actor;
			record.status = {};
			record.status.operation_id = command.operation_id;
			record.status.phase =
				player_death_restitution_runtime_operation_phase::unknown;
			record.status.durability = critical_command_durability::unknown;
			record.status.completion_outcome =
				critical_apply_outcome::retryable_failure;
			record.status.exact_verification_required = true;
			return &record;
		}
		catch (const std::bad_alloc &)
		{
			record = {};
			return nullptr;
		}
	}
	for (auto &record : operation_statuses)
	{
		if (!record.in_use || record.status.target_save_login_fence_held ||
		    !record.status.completion_available)
			continue;
		// Completed projections are a bounded reconnect cache.  Evicting one
		// never affects the durable operation; status then falls back to the
		// protected exact verification path.
		record = {};
		try
		{
			record.in_use = true;
			record.operation_id = command.operation_id;
			record.plan_digest = plan.plan_digest;
			record.schema_version = command.schema_version;
			record.payload_version = command.payload_version;
			record.source_site = command.source_site;
			record.deadline_class = command.deadline_class;
			record.accepted_at_usec = command.accepted_at_usec;
			record.actor = actor;
			record.status.operation_id = command.operation_id;
			record.status.phase =
				player_death_restitution_runtime_operation_phase::unknown;
			record.status.durability = critical_command_durability::unknown;
			record.status.completion_outcome =
				critical_apply_outcome::retryable_failure;
			record.status.exact_verification_required = true;
			return &record;
		}
		catch (const std::bad_alloc &)
		{
			record = {};
			return nullptr;
		}
	}
	return nullptr;
}

void release_status(operation_status_record *record)
{
	if (record)
		*record = {};
}

bool status_identity_matches(const operation_status_record &record, const critical_command &command,
			     const player_death_restitution_plan &plan, const char *actor)
{
	return actor && record.actor == actor &&
	       record.operation_id.bytes == command.operation_id.bytes &&
	       record.plan_digest == plan.plan_digest &&
	       record.schema_version == command.schema_version &&
	       record.payload_version == command.payload_version &&
	       record.source_site == command.source_site &&
	       record.deadline_class == command.deadline_class &&
	       record.accepted_at_usec == command.accepted_at_usec;
}

bool result_keeps_operation(player_death_restitution_runtime_result result)
{
	return result == player_death_restitution_runtime_result::accepted ||
	       result == player_death_restitution_runtime_result::awaiting_durability ||
	       result == player_death_restitution_runtime_result::attached ||
	       result == player_death_restitution_runtime_result::journal_uncertain;
}

void update_status_from_submission(operation_status_record &record,
				   player_death_restitution_runtime_result result,
				   const player_death_restitution_runtime_submission &submission)
{
	player_death_restitution_runtime_operation_status &status = record.status;
	status.operation_id = submission.operation_id;
	status.completion_available = false;
	status.durable_receipt_recorded = false;
	status.target_save_login_fence_held = submission.target_save_login_fence_held;
	status.exact_verification_required = true;
	status.retry_safe = false;
	status.completion_outcome = critical_apply_outcome::retryable_failure;
	switch (result)
	{
	case player_death_restitution_runtime_result::awaiting_durability:
		status.phase =
			player_death_restitution_runtime_operation_phase::awaiting_durability;
		status.durability = critical_command_durability::awaiting_durability;
		return;
	case player_death_restitution_runtime_result::accepted:
		status.phase = player_death_restitution_runtime_operation_phase::admitted;
		status.durability = critical_command_durability::unknown;
		return;
	case player_death_restitution_runtime_result::attached:
		status.phase = player_death_restitution_runtime_operation_phase::admitted;
		status.durability = critical_command_durability::unknown;
		return;
	case player_death_restitution_runtime_result::journal_uncertain:
		status.phase = player_death_restitution_runtime_operation_phase::journal_uncertain;
		status.durability = critical_command_durability::uncertain;
		return;
	default:
		status.phase = player_death_restitution_runtime_operation_phase::unknown;
		status.durability = critical_command_durability::unknown;
		return;
	}
}

void update_status_from_completion(operation_status_record &record,
				   const critical_completion &completion)
{
	player_death_restitution_runtime_operation_status &status = record.status;
	status.operation_id = completion.operation_id;
	status.completion_available = true;
	status.completion_outcome = completion.outcome;
	status.target_save_login_fence_held = true;
	status.exact_verification_required = true;
	status.retry_safe = false;
	status.durable_receipt_recorded = false;
	if (completion.outcome == critical_apply_outcome::applied ||
	    completion.outcome == critical_apply_outcome::already_applied)
	{
		status.phase =
			player_death_restitution_runtime_operation_phase::durable_receipt_unverified;
		status.durability = critical_command_durability::durable;
		player_death_restitution_result result = {};
		status.durable_receipt_recorded =
			completion.result_size == PLAYER_DEATH_RESTITUTION_RESULT_BYTES &&
			player_death_restitution_command_decode_result(
				completion.result_payload.data(), completion.result_size,
				&result) &&
			critical_operation_id_equal(result.restitution_id,
						    completion.operation_id) &&
			result.mutation_applied;
		return;
	}
	if (completion.outcome == critical_apply_outcome::terminal_failure)
	{
		status.phase = player_death_restitution_runtime_operation_phase::terminal_failure;
		status.durability = critical_command_durability::failed;
		return;
	}
	status.phase = player_death_restitution_runtime_operation_phase::durable_execution;
	status.durability = critical_command_durability::durable;
}

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
	if (!player_death_restitution_plan_valid(plan))
		return player_death_restitution_runtime_result::invalid_plan;
	critical_command preview = {};
	if (!player_death_restitution_command_build(plan, &preview))
		return player_death_restitution_runtime_result::invalid_plan;
	player_death_restitution_plan normalized = plan;
	normalized.accepted_at_usec = preview.accepted_at_usec;
	if (operation_status_record *existing = find_status(normalized.restitution_id))
	{
		if (!status_identity_matches(*existing, preview, normalized,
					     normalized.actor.c_str()))
			return player_death_restitution_runtime_result::identity_conflict;
		if (player_death_restitution_runtime_submission *live =
			    find_submission(normalized.restitution_id))
		{
			if (submission_out)
				*submission_out = *live;
		}
		else if (submission_out)
			submission_out->operation_id = normalized.restitution_id;
		return player_death_restitution_runtime_result::attached;
	}
	operation_status_record *status =
		reserve_status(preview, normalized, normalized.actor.c_str());
	if (!status)
		return player_death_restitution_runtime_result::overloaded;
	player_death_restitution_runtime_submission *slot = find_free_submission();
	if (!slot)
	{
		release_status(status);
		return player_death_restitution_runtime_result::overloaded;
	}

	player_death_restitution_runtime_submission submission = {};
	const player_death_restitution_runtime_result result =
		player_death_restitution_runtime_submit(normalized, live_callbacks, nullptr,
							&submission);
	if (result_keeps_operation(result))
	{
		*slot = submission;
		update_status_from_submission(*status, result, submission);
		if (submission_out)
			*submission_out = submission;
	}
	else
		release_status(status);
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
	if (operation_status_record *existing = find_status(approved_command.operation_id))
	{
		if (!status_identity_matches(*existing, approved_command, plan, actor))
			return player_death_restitution_runtime_result::identity_conflict;
		if (player_death_restitution_runtime_submission *live =
			    find_submission(approved_command.operation_id))
		{
			if (submission_out)
				*submission_out = *live;
		}
		else if (submission_out)
			submission_out->operation_id = approved_command.operation_id;
		return player_death_restitution_runtime_result::attached;
	}
	operation_status_record *status = reserve_status(approved_command, plan, actor);
	if (!status)
		return player_death_restitution_runtime_result::overloaded;
	player_death_restitution_runtime_submission *slot = find_free_submission();
	if (!slot)
	{
		release_status(status);
		return player_death_restitution_runtime_result::overloaded;
	}
	player_death_restitution_runtime_submission submission = {};
	const player_death_restitution_runtime_result result =
		player_death_restitution_runtime_submit_command(approved_command, live_callbacks,
								nullptr, &submission);
	if (result_keeps_operation(result))
	{
		*slot = submission;
		update_status_from_submission(*status, result, submission);
		if (submission_out)
			*submission_out = submission;
	}
	else
		release_status(status);
	return result;
}

bool player_death_restitution_runtime_restore_replayed_command(const critical_command &command,
							       void *)
{
	if (command.type != critical_command_type::player_death_restitution)
		return true;
	player_death_restitution_plan plan = {};
	if (!critical_command_valid(command) ||
	    !player_death_restitution_command_decode_payload(command, &plan))
		return false;
	if (operation_status_record *existing = find_status(command.operation_id))
	{
		if (!status_identity_matches(*existing, command, plan, plan.actor.c_str()))
			return false;
		return find_submission(command.operation_id) != nullptr;
	}
	player_death_restitution_runtime_submission *slot = find_free_submission();
	if (!slot)
		return false;
	operation_status_record *status = reserve_status(command, plan, plan.actor.c_str());
	if (!status)
		return false;
	player_death_restitution_runtime_submission submission = {};
	const player_death_restitution_runtime_result result =
		player_death_restitution_runtime_restore_replayed_command(command, live_callbacks,
									  nullptr, &submission);
	if (result != player_death_restitution_runtime_result::accepted)
	{
		release_status(status);
		return false;
	}
	*slot = submission;
	update_status_from_submission(*status, result, submission);
	return true;
}

bool player_death_restitution_runtime_operation_status_copy(
	const char *actor, int actor_level, const critical_operation_id &operation_id,
	player_death_restitution_runtime_operation_status *status_out)
{
	if (status_out)
		*status_out = {};
	if (!actor || !*actor || actor_level < FORGER || strnlen(actor, 129) > 128 ||
	    critical_operation_id_is_zero(operation_id) || !status_out)
		return false;
	operation_status_record *record = find_status(operation_id);
	if (!record || record->actor != actor)
		return false;
	*status_out = record->status;
	return true;
}

void player_death_restitution_runtime_handle_completions(const critical_completion *completions,
							 size_t count)
{
	if (!completions)
		return;
	for (size_t index = 0; index < count; ++index)
	{
		operation_status_record *status = find_status(completions[index].operation_id);
		if (status)
			update_status_from_completion(*status, completions[index]);
		player_death_restitution_runtime_submission *submission =
			find_submission(completions[index].operation_id);
		if (submission &&
		    player_death_restitution_runtime_complete(submission, completions[index],
							      live_callbacks, nullptr) &&
		    status)
			status->status.target_save_login_fence_held = false;
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
