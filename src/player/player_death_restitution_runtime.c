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
	case critical_submit_result::awaiting_durability:
		return player_death_restitution_runtime_result::awaiting_durability;
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

player_death_restitution_runtime_result player_death_restitution_runtime_restore_replayed_command(
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

#include <limits>
#include <iterator>

namespace
{
using restitution_replay_reserve_fn = bool (*)(size_t, void *) noexcept;
struct restitution_runtime_replay_workspace
{
	player_death_restitution_plan plan = {};
	size_t plan_heap = 0;
	size_t base = 0;
	size_t current = 0;
	size_t requested = 0;
	restitution_replay_reserve_fn reserve = nullptr;
	void *context = nullptr;

	bool admit(size_t request) noexcept
	{
		if (plan_heap > std::numeric_limits<size_t>::max() - base)
			return false;
		current = base + plan_heap;
		if (request > std::numeric_limits<size_t>::max() - current)
			return false;
		requested = current + request;
		return reserve && reserve(requested, context);
	}
};

// Complete source-declared carrier allowance for the actual allocation-free
// schema1 command-valid path, including original none_of and binary_search.
// Pinned installed GCC13 algorithms loop; they neither recurse nor allocate.
constexpr size_t restitution_command_valid_frames =
	// valid/legacy/envelope: command references, returned bools, zero-ID byte
	// scan and encoded-size bytes/add lambda/count/width/result carriers.
	8 * sizeof(void *) + 8 * sizeof(size_t) + 4 * sizeof(bool) + sizeof(uint8_t) +
	// Original two envelope loops index/key-reference, original comparator
	// key references/result and native-auction/key-limit pure scalar returns.
	7 * sizeof(void *) + 3 * sizeof(size_t) + 3 * sizeof(bool) +
	// none_of -> find_if -> __find_if wrappers: first/last/result iterators,
	// empty original predicate/wrapper objects and random-access trip count.
	12 * sizeof(std::vector<critical_entity_key>::const_iterator) + 4 * sizeof(char) +
	2 * sizeof(std::random_access_iterator_tag) + sizeof(std::ptrdiff_t) +
	// binary_search -> __lower_bound: first/last/middle/result iterators,
	// key refs and function-pointer comparator adapters; len/half/distance
	// and advance carriers. Temporally separate helper calls sum safely.
	9 * sizeof(std::vector<critical_entity_key>::const_iterator) + 6 * sizeof(void *) +
	6 * sizeof(std::ptrdiff_t) + 3 * sizeof(bool);
}

bool player_death_restitution_runtime_replay_budget_owner::command_valid_bounded(
	const critical_command &command, reserve_fn reserve, void *context,
	size_t outer_live) noexcept
{
	constexpr size_t own_carriers = 3 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
	return reserve &&
	       outer_live <= std::numeric_limits<size_t>::max() - own_carriers -
				     restitution_command_valid_frames &&
	       reserve(outer_live + own_carriers + restitution_command_valid_frames, context) &&
	       critical_command_valid(command);
}

player_death_restitution_runtime_result
player_death_restitution_runtime_replay_budget_owner::restore(
	const critical_command &command,
	const player_death_restitution_runtime_callbacks &original_callbacks,
	void *original_context, const callbacks &bounded_callbacks, reserve_fn reserve,
	void *budget_context, size_t outer_live,
	player_death_restitution_runtime_submission *submission) noexcept
{
	constexpr size_t parameter_carriers = 7 * sizeof(void *) + sizeof(size_t);
	if (!submission || !reserve ||
	    outer_live > std::numeric_limits<size_t>::max() -
				 sizeof(restitution_runtime_replay_workspace) - parameter_carriers)
		return player_death_restitution_runtime_result::invalid_plan;
	restitution_runtime_replay_workspace work;
	work.base = outer_live + sizeof(work) + parameter_carriers;
	work.reserve = reserve;
	work.context = budget_context;
	if (!work.admit(sizeof(player_death_restitution_runtime_submission) + sizeof(void *)))
		return player_death_restitution_runtime_result::invalid_plan;
	clear_submission(submission);
	if (!command_valid_bounded(command, reserve, budget_context, work.current) ||
	    command.type != critical_command_type::player_death_restitution)
		return player_death_restitution_runtime_result::invalid_plan;
	if (!work.admit(0) ||
	    !player_death_restitution_command_decode_payload_bounded(
		    command, &work.plan, reserve, budget_context, work.current, &work.plan_heap))
		return player_death_restitution_runtime_result::invalid_plan;
	// This is the original third full validation within the runtime: its own
	// decode validates twice before this preflight. The adapter's earlier two
	// decode validations also remain. Preserve all five original passes.
	if (!work.admit(sizeof(void *) + sizeof(bool)) ||
	    !player_death_restitution_plan_valid_bounded(work.plan, reserve, budget_context,
							 work.current) ||
	    !callbacks_complete(original_callbacks))
		return player_death_restitution_runtime_result::invalid_plan;
	if (!work.admit(0) ||
	    !bounded_callbacks.recipient_is_offline(work.plan.recipient_pid, original_context,
						    reserve, budget_context, work.current))
		return player_death_restitution_runtime_result::recipient_online;
	if (!work.admit(0) ||
	    !bounded_callbacks.pending_save_is_empty(
		    work.plan.recipient_pid, work.plan.expected_recipient_save_revision,
		    original_context, reserve, budget_context, work.current))
		return player_death_restitution_runtime_result::pending_save;
	// The concrete adapter callback preadmits the entire fixed original
	// offline/pipeline-acquire/offline/exact-release closure before entering.
	if (!work.admit(0) ||
	    !bounded_callbacks.acquire_target_save_login_fence(
		    work.plan.recipient_pid, work.plan.expected_recipient_save_revision,
		    original_context, reserve, budget_context, work.current))
		return player_death_restitution_runtime_result::fence_unavailable;
	// Original nonfallible post-acquire tail. No admission callback, heap
	// allocation, new predicate, journal action or submission follows.
	submission->operation_id = command.operation_id;
	submission->recipient_pid = work.plan.recipient_pid;
	submission->expected_save_revision = work.plan.expected_recipient_save_revision;
	submission->target_save_login_fence_held = true;
	return player_death_restitution_runtime_result::accepted;
}
