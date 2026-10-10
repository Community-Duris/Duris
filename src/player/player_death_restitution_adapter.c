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

// This owner is deliberately private to the genuine future adapter replay
// provider. The original live/replay paths above are unchanged and unselected.
class player_death_restitution_replay_budget_owner;
class player_death_restitution_status_cache_budget_owner
{
	friend class player_death_restitution_replay_budget_owner;
	friend bool
	player_death_restitution_runtime_replay_storage_source_frames(size_t *) noexcept;
	friend bool player_death_restitution_runtime_replay_storage_bytes(size_t *) noexcept;

	using reserve_fn = bool (*)(size_t, void *) noexcept;
	struct census
	{
		size_t total = sizeof(live_submissions) + sizeof(operation_statuses) +
			       sizeof(live_callbacks);
		size_t index = 0;
		size_t capacity = 0;

		bool observe() noexcept
		{
			total = sizeof(live_submissions) + sizeof(operation_statuses) +
				sizeof(live_callbacks);
			for (index = 0; index < operation_statuses.size(); ++index)
			{
				// Empty/inactive records can retain their old allocation after
				// record = {}. Capacity, rather than in_use or size, owns it.
				capacity = operation_statuses[index].actor.capacity();
				if (capacity <= 15)
					continue;
				if (capacity == std::numeric_limits<size_t>::max() ||
				    capacity + 1 > std::numeric_limits<size_t>::max() - total)
					return false;
				total += capacity + 1;
			}
			return true;
		}
	};

	struct workspace
	{
		census cache;
		size_t base = 0;
		size_t current = 0;
		size_t requested = 0;
		size_t length = 0;
		size_t new_capacity = 0;
		size_t index = 0;
		size_t id_index = 0;
		operation_status_record *record = nullptr;
		bool evict = false;
		bool zero = true;
		reserve_fn reserve = nullptr;
		void *context = nullptr;

		bool admit(size_t request) noexcept
		{
			if (!cache.observe() ||
			    cache.total > std::numeric_limits<size_t>::max() - base)
				return false;
			current = base + cache.total;
			if (request > std::numeric_limits<size_t>::max() - current)
				return false;
			requested = current + request;
			return reserve && reserve(requested, context);
		}
	};

	static bool profile() noexcept
	{
#if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG)
		return true;
#else
		return false;
#endif
	}

	// Incoming full outer owns command/plan/input/caller/C and sibling owners,
	// and excludes exactly the three owned static objects observed above.
	// This method owns its real workspace and prospective reset/assignment
	// carriers. It is not a lease for later calls or a historical peak.
	static bool reserve_status_bounded(const critical_command &command,
					   const player_death_restitution_plan &plan,
					   const char *actor, reserve_fn reserve, void *context,
					   size_t outer_live, operation_status_record **record_out,
					   size_t *current_cache_out) noexcept
	{
		if (!record_out || !current_cache_out || !reserve || !profile() ||
		    outer_live > std::numeric_limits<size_t>::max() - sizeof(workspace))
			return false;
		workspace work;
		work.base = outer_live + sizeof(workspace);
		work.reserve = reserve;
		work.context = context;
		if (!work.admit(0))
			return false;
		// Same original first-NUL actor and zero-ID refusals, with no hidden
		// allocating identity helper or temporary string.
		if (!actor || !*actor)
		{
			*record_out = nullptr;
			*current_cache_out = work.cache.total;
			return true;
		}
		for (work.id_index = 0; work.id_index < command.operation_id.bytes.size();
		     ++work.id_index)
			if (command.operation_id.bytes[work.id_index])
				work.zero = false;
		if (work.zero)
		{
			*record_out = nullptr;
			*current_cache_out = work.cache.total;
			return true;
		}
		// Preserve duplicate precedence over free/completed rows.
		for (work.index = 0; work.index < operation_statuses.size(); ++work.index)
		{
			work.record = &operation_statuses[work.index];
			if (!work.record->in_use)
				continue;
			for (work.id_index = 0; work.id_index < command.operation_id.bytes.size();
			     ++work.id_index)
				if (work.record->operation_id.bytes[work.id_index] !=
				    command.operation_id.bytes[work.id_index])
					break;
			if (work.id_index == command.operation_id.bytes.size())
			{
				*record_out = work.record;
				*current_cache_out = work.cache.total;
				return true;
			}
		}
		work.record = nullptr;
		for (work.index = 0; work.index < operation_statuses.size(); ++work.index)
			if (!operation_statuses[work.index].in_use)
			{
				work.record = &operation_statuses[work.index];
				break;
			}
		if (!work.record)
			for (work.index = 0; work.index < operation_statuses.size(); ++work.index)
				if (operation_statuses[work.index].in_use &&
				    !operation_statuses[work.index]
					     .status.target_save_login_fence_held &&
				    operation_statuses[work.index].status.completion_available)
				{
					work.record = &operation_statuses[work.index];
					work.evict = true;
					break;
				}
		if (!work.record)
		{
			*record_out = nullptr;
			*current_cache_out = work.cache.total;
			return true;
		}
		// Original operator=(const char*) uses char_traits::length: a protected
		// actor containing NUL stores only its prefix, not plan.actor.size().
		while (actor[work.length])
			++work.length;
		if (work.length > work.record->actor.max_size())
			return false;
		work.requested = 0;
		if (work.length > work.record->actor.capacity())
		{
			work.new_capacity = work.length;
			if (work.record->actor.capacity() > std::numeric_limits<size_t>::max() / 2)
				return false;
			if (work.length < 2 * work.record->actor.capacity())
			{
				work.new_capacity = 2 * work.record->actor.capacity();
				if (work.new_capacity > work.record->actor.max_size())
					work.new_capacity = work.record->actor.max_size();
			}
			if (work.new_capacity == std::numeric_limits<size_t>::max())
				return false;
			work.requested = work.new_capacity + 1;
		}
		// Admit the new allocation while the real old cache heap still lives,
		// and admit the original record/status reset carriers before any
		// mutation. Cleanup will not call a fallible reserve after refusal.
		// _M_replace/_M_mutate/_M_create actual source fixed scalar carriers:
		// old/new size, pos/len1/len2, how_much/new_capacity, pointer/result,
		// capacity reference and old_capacity. Runtime assign is disjoint
		// (actor belongs to the decoded plan), so cold overlapping copy is
		// not a hidden path. These are source expressions, not a heap cap.
		constexpr size_t assignment_frames =
			// operator=(const char*) -> assign(const char*): this/source and
			// reference result carriers; char_traits::length pointer/result.
			6 * sizeof(void *) + sizeof(size_t) +
			// _M_replace: this/source, pos/len1/len2, old/new sizes,
			// in-capacity p/how_much (included though allocation disjoint).
			3 * sizeof(void *) + 6 * sizeof(size_t) +
			// _M_mutate: this/source, pos/len1/len2, how_much/new_capacity/r.
			3 * sizeof(void *) + 5 * sizeof(size_t) +
			// _M_create: this/capacity-reference/returned allocation, old cap.
			3 * sizeof(void *) + sizeof(size_t) +
			// _S_allocate and allocator argument/returned-pointer carriers.
			4 * sizeof(void *) + 2 * sizeof(size_t) +
			// _S_copy / char_traits::copy: destination/source/count/result.
			5 * sizeof(void *) + 2 * sizeof(size_t);
		constexpr size_t cleanup_frames =
			sizeof(operation_status_record) +
			sizeof(player_death_restitution_runtime_operation_status);
		if (work.requested > std::numeric_limits<size_t>::max() - assignment_frames -
					     cleanup_frames ||
		    !work.admit(work.requested + assignment_frames + cleanup_frames))
			return false;
		if (work.evict)
			*work.record = {};
		try
		{
			work.record->in_use = true;
			work.record->operation_id = command.operation_id;
			work.record->plan_digest = plan.plan_digest;
			work.record->schema_version = command.schema_version;
			work.record->payload_version = command.payload_version;
			work.record->source_site = command.source_site;
			work.record->deadline_class = command.deadline_class;
			work.record->accepted_at_usec = command.accepted_at_usec;
			work.record->actor = actor;
			if (!work.evict)
				work.record->status = {};
			work.record->status.operation_id = command.operation_id;
			work.record->status.phase =
				player_death_restitution_runtime_operation_phase::unknown;
			work.record->status.durability = critical_command_durability::unknown;
			work.record->status.completion_outcome =
				critical_apply_outcome::retryable_failure;
			work.record->status.exact_verification_required = true;
		}
		catch (const std::bad_alloc &)
		{
			*work.record = {};
			work.record = nullptr;
		}
		// Actual same-owner observation, not stale preallocation size. All
		// admitted actor capacities are representable, including failed
		// resets. There is no allocating or fallible admission here.
		// Representability is established above: all unchanged heaps were in
		// the admitted old total and the only new capacity was admitted while
		// that old total still lived. This game-thread owner admits no cache
		// concurrency. Fresh census is therefore strong and cannot overflow.
		work.cache.observe();
		*record_out = work.record;
		*current_cache_out = work.cache.total;
		return true;
	}

	// Caller has already admitted the full record={} cleanup carrier while
	// it owned the returned status. This is the original no-allocation reset.
	static void release_status_preallowed(operation_status_record *record) noexcept
	{
		release_status(record);
	}
};

bool player_death_restitution_runtime_replay_storage_bytes(size_t *output) noexcept
{
	if (!output || !player_death_restitution_status_cache_budget_owner::profile())
		return false;
	player_death_restitution_status_cache_budget_owner::census observed;
	if (!observed.observe())
		return false;
	*output = observed.total;
	return true;
}

#include <deque>
#include <mutex>
#include "player/player_revision_state.h"

// The concrete adapter is the only friend capable of constructing the paired
// private runtime callback table. It forwards to the ACTUAL original callbacks
// only after their entire allocation-free source closure has been admitted.
class player_death_restitution_replay_budget_owner
{
	friend bool player_death_restitution_runtime_restore_replayed_command_bounded(
		const critical_command &, void *, bool (*)(size_t, void *) noexcept, void *,
		size_t) noexcept;
	using reserve_fn = bool (*)(size_t, void *) noexcept;
	using cache_owner = player_death_restitution_status_cache_budget_owner;
	using runtime_owner = player_death_restitution_runtime_replay_budget_owner;

	// Exact supported ordinary GCC13 lookup profile. These are prospective
	// SOURCE carrier expressions, not claims about emitted machine stack.
	// Source calls iterate fixed arrays/deques/linked lists and map/set lookups.
	// None constructs or copies a snapshot, string, native object or queue.
	static constexpr size_t deque_read_frames =
		// Actual two range iterators plus begin/end returned carriers; actual
		// _Deque_iterator has four pointer fields for every element type.
		4 * sizeof(std::deque<player_load_result>::iterator) +
		// job/result reference, iterator ++/set_node and comparison arguments.
		8 * sizeof(void *) + 2 * sizeof(size_t);
	static constexpr size_t hash_read_frames =
		// unordered_map::find, _Hashtable::find code/bucket/iterator carriers.
		7 * sizeof(void *) + 2 * sizeof(size_t) +
		// _M_find_node/_M_find_before_node this/key/node/prev/returned pointers.
		9 * sizeof(void *) + 4 * sizeof(size_t) +
		// _M_equals/_M_key_equals, hash/extract/equal and bucket-index calls.
		12 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(int) + 3 * sizeof(bool) +
		2 * sizeof(char);
	static constexpr size_t set_read_frames =
		// set.count calls _Rb_tree.find, NOT _Rb_tree.count/equal_range.
		// Its actual find iterator and iterative lower_bound x/y/key carriers,
		// begin/end/key/left/right/less/iterator comparison and result carriers.
		23 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + sizeof(char);
	static constexpr size_t fixed_array_find_frames =
		// Real pid parameter, two array-range pointers, current reference and
		// returned pointer, with no copying of the pointed-to owner.
		sizeof(int) + 4 * sizeof(void *);
	static constexpr size_t revision_read_frames =
		// snapshot_copy pid/out/state, actual fixed aggregate return carrier,
		// find_state pid/found iterator plus original map lookup closure.
		2 * sizeof(int) + 3 * sizeof(void *) + sizeof(player_revision_snapshot) +
		hash_read_frames + sizeof(bool);
	static constexpr size_t quarantine_read_frames =
		sizeof(int) + sizeof(std::lock_guard<std::mutex>) + set_read_frames + sizeof(bool);
	static constexpr size_t worker_read_frames =
		sizeof(int) + sizeof(std::lock_guard<std::mutex>) + sizeof(void *) +
		hash_read_frames + sizeof(bool);
	static constexpr size_t retained_read_frames =
		// append_retry pointer inspection and the two real retained deques.
		sizeof(int) + 2 * deque_read_frames + sizeof(bool);
	static constexpr size_t offline_frames =
		// Original recipient callback parameters and target_pid, descriptor,
		// actual candidates[2], range pointers and character reference.
		sizeof(uint32_t) + sizeof(int) + 8 * sizeof(void *) + sizeof(bool) +
		// Original load predicate: pid, lock and queued/completion range reads.
		sizeof(int) + sizeof(std::lock_guard<std::mutex>) + 2 * deque_read_frames +
		sizeof(bool) +
		// is_pid_online(true): actual pid/includeLD/temp_ch and scalar return.
		sizeof(int) + 2 * sizeof(bool) + sizeof(void *);
	static constexpr size_t pending_frames =
		// Original adapter pending callback by-value parameters/result.
		sizeof(uint32_t) + sizeof(uint64_t) + sizeof(void *) + sizeof(bool) +
		// Actual pipeline pending pid, lock, revision object and fixed lookups.
		sizeof(int) + sizeof(std::lock_guard<std::mutex>) +
		sizeof(player_revision_snapshot) + sizeof(bool) + 2 * fixed_array_find_frames +
		retained_read_frames + quarantine_read_frames + worker_read_frames +
		revision_read_frames;
	static constexpr size_t pipeline_release_frames =
		sizeof(int) + sizeof(uint64_t) + sizeof(std::lock_guard<std::mutex>) +
		sizeof(void *) + fixed_array_find_frames +
		// Original target_save_login_fence={} carrier: supported LP64 fields
		// int4, padding4, uint64 revision8. No mirror struct or fake instance.
		2 * sizeof(uint64_t);
	static constexpr size_t pipeline_acquire_frames =
		sizeof(int) + sizeof(uint64_t) + 2 * sizeof(std::lock_guard<std::mutex>) +
		sizeof(player_revision_snapshot) + sizeof(void *) + sizeof(bool) +
		quarantine_read_frames + worker_read_frames + revision_read_frames +
		// terminal, target, literal, allocate-target's nested target lookup
		// and own range, then final-target. Allocate's revision parameter is
		// an additional actual by-value carrier alongside its pid parameter.
		6 * fixed_array_find_frames + sizeof(uint64_t) + 2 * retained_read_frames +
		pipeline_release_frames + 2 * sizeof(uint64_t);
	static constexpr size_t acquire_frames =
		sizeof(uint32_t) + sizeof(uint64_t) + sizeof(void *) + sizeof(bool) +
		2 * offline_frames + pipeline_acquire_frames + pipeline_release_frames;
	static constexpr size_t bounded_callback_carriers = sizeof(uint32_t) + sizeof(uint64_t) +
							    3 * sizeof(void *) + sizeof(size_t) +
							    sizeof(bool);

	static bool callback_admit(reserve_fn reserve, void *context, size_t outer,
				   size_t frames) noexcept
	{
		constexpr size_t own_carriers =
			2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
		return reserve && frames <= std::numeric_limits<size_t>::max() - own_carriers &&
		       outer <= std::numeric_limits<size_t>::max() - frames - own_carriers &&
		       reserve(outer + frames + own_carriers, context);
	}
	static bool offline_bounded(uint32_t pid, void *original_context, reserve_fn reserve,
				    void *context, size_t outer) noexcept
	{
		return callback_admit(reserve, context, outer,
				      bounded_callback_carriers + offline_frames) &&
		       recipient_is_offline(pid, original_context);
	}
	static bool pending_bounded(uint32_t pid, uint64_t revision, void *original_context,
				    reserve_fn reserve, void *context, size_t outer) noexcept
	{
		return callback_admit(reserve, context, outer,
				      bounded_callback_carriers + pending_frames) &&
		       pending_save_is_empty(pid, revision, original_context);
	}
	static bool acquire_bounded(uint32_t pid, uint64_t revision, void *original_context,
				    reserve_fn reserve, void *context, size_t outer) noexcept
	{
		// Entire original first offline, acquisition (including failure
		// release), second offline and exact second-refusal release admitted
		// BEFORE the actual fence. No post-acquire budget callback is possible.
		return callback_admit(reserve, context, outer,
				      bounded_callback_carriers + acquire_frames) &&
		       acquire_target_save_login_fence(pid, revision, original_context);
	}

	struct workspace
	{
		player_death_restitution_plan plan = {};
		player_death_restitution_runtime_submission submission = {};
		cache_owner::census cache;
		const runtime_owner::callbacks bounded = {
			offline_bounded,
			pending_bounded,
			acquire_bounded,
		};
		size_t plan_heap = 0;
		size_t base = 0;
		size_t current = 0;
		size_t requested = 0;
		size_t cache_after_reservation = 0;
		operation_status_record *status = nullptr;
		player_death_restitution_runtime_submission *slot = nullptr;
		player_death_restitution_runtime_result result =
			player_death_restitution_runtime_result::invalid_plan;
		reserve_fn reserve = nullptr;
		void *context = nullptr;

		bool observe() noexcept
		{
			if (!cache.observe() ||
			    plan_heap > std::numeric_limits<size_t>::max() - base)
				return false;
			current = base + plan_heap;
			if (cache.total > std::numeric_limits<size_t>::max() - current)
				return false;
			current += cache.total;
			return true;
		}
		bool admit(size_t request) noexcept
		{
			if (!observe() || request > std::numeric_limits<size_t>::max() - current)
				return false;
			requested = current + request;
			return reserve && reserve(requested, context);
		}
	};

	static bool restore(const critical_command &command, void *original_context,
			    reserve_fn reserve, void *context, size_t outer_live) noexcept
	{
		(void)original_context; // Original adapter ignores its host callback context.
		// Preserve the original host family skip BEFORE capability admission.
		if (command.type != critical_command_type::player_death_restitution)
			return true;
		constexpr size_t entry_carriers =
			2 * (4 * sizeof(void *) + sizeof(size_t) + sizeof(bool));
		constexpr size_t lookup_frames = 14 * sizeof(void *) + 4 * sizeof(size_t) +
						 4 * sizeof(bool) + sizeof(uint8_t);
		constexpr size_t identity_frames =
			12 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool);
		constexpr size_t status_call_carriers = 7 * sizeof(void *) + sizeof(size_t);
		constexpr size_t cleanup_frames =
			sizeof(operation_status_record) + 2 * sizeof(void *);
		constexpr size_t success_tail_frames =
			4 * sizeof(void *) + sizeof(player_death_restitution_runtime_result) +
			sizeof(player_death_restitution_runtime_submission);
		if (!reserve || !cache_owner::profile() || sizeof(void *) != 8 ||
		    sizeof(int) != 4 || alignof(uint64_t) != 8 ||
		    outer_live >
			    std::numeric_limits<size_t>::max() - sizeof(workspace) - entry_carriers)
			return false;
		workspace work;
		work.base = outer_live + sizeof(work) + entry_carriers;
		work.reserve = reserve;
		work.context = context;
		if (!work.admit(0) ||
		    !runtime_owner::command_valid_bounded(command, reserve, context,
							  work.current) ||
		    !work.admit(0) ||
		    !player_death_restitution_command_decode_payload_bounded(
			    command, &work.plan, reserve, context, work.current, &work.plan_heap))
			return false;
		if (!work.admit(lookup_frames + identity_frames))
			return false;
		if ((work.status = find_status(command.operation_id)))
		{
			if (!status_identity_matches(*work.status, command, work.plan,
						     work.plan.actor.c_str()))
				return false;
			return find_submission(command.operation_id) != nullptr;
		}
		work.slot = find_free_submission();
		if (!work.slot)
			return false;
		// Incoming cache-owner outer excludes exactly its static arrays/table
		// and actor heaps, and includes actual adapter plan/workspace/call and
		// future cleanup carriers. Census reobserves every owned capacity.
		if (work.plan_heap > std::numeric_limits<size_t>::max() - work.base ||
		    work.base + work.plan_heap > std::numeric_limits<size_t>::max() -
							 status_call_carriers - cleanup_frames -
							 success_tail_frames ||
		    !cache_owner::reserve_status_bounded(
			    command, work.plan, work.plan.actor.c_str(), reserve, context,
			    work.base + work.plan_heap + status_call_carriers + cleanup_frames +
				    success_tail_frames,
			    &work.status, &work.cache_after_reservation) ||
		    !work.status)
			return false;
		if (!work.observe())
		{
			cache_owner::release_status_preallowed(work.status);
			return false;
		}
		work.result = runtime_owner::restore(command, live_callbacks, nullptr, work.bounded,
						     reserve, context, work.current,
						     &work.submission);
		if (work.result != player_death_restitution_runtime_result::accepted)
		{
			cache_owner::release_status_preallowed(work.status);
			return false;
		}
		// Exact original nonfallible success/removal tail; fence is now real.
		// Cache actor storage did not mutate during runtime's pure decode and
		// actual read-only/fixed-slot callbacks. No admission follows success.
		*work.slot = work.submission;
		update_status_from_submission(*work.status, work.result, work.submission);
		return true;
	}
};

bool player_death_restitution_runtime_restore_replayed_command_bounded(
	const critical_command &command, void *original_context,
	bool (*reserve)(size_t, void *) noexcept, void *budget_context, size_t outer_live) noexcept
{
	return player_death_restitution_replay_budget_owner::restore(
		command, original_context, reserve, budget_context, outer_live);
}

namespace
{
constexpr size_t restitution_current_source_P = sizeof(void *),
		 restitution_current_source_N = sizeof(size_t),
		 restitution_current_source_B = sizeof(bool);
// Actual census.observe: receiver/result, array size, direct mutable operator[],
// complete string capacity/is_local/local_data/pointer_traits/address chain,
// and two numeric_limits<size_t>::max results. Fresh array has no _S_ref helper.
constexpr size_t restitution_current_census_observe_source = 16 * restitution_current_source_P +
							     5 * restitution_current_source_N +
							     2 * restitution_current_source_B;
// CURRENT output/result + actual profile result + census ctor/dtor receivers.
// Actual census object sizeof is added by its friend getter; its fields must not
// be duplicated by a guessed DTO or by a fixed-record-count frame multiplier.
constexpr size_t restitution_current_other_source = 3 * restitution_current_source_P +
						    2 * restitution_current_source_B +
						    restitution_current_census_observe_source;
}

bool player_death_restitution_runtime_replay_storage_source_frames(size_t *output) noexcept
{
#if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&             \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                      \
	__cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) && \
	!defined(_GLIBCXX_PARALLEL)
	if (!output)
		return false;
	*output = sizeof(player_death_restitution_status_cache_budget_owner::census) +
		  restitution_current_other_source;
	return true;
#else
	(void)output;
	return false;
#endif
}
