#include "corpse_lifecycle_transaction.h"

#include "item_transfer_command.h"

#include <cerrno>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
struct corpse_state
{
	uint32_t owner_pid = 0;
	uint32_t save_id = 0;
	uint64_t revision = 0;
	corpse_lifecycle_payload desired = {};
	corpse_lifecycle_payload inflight = {};
	critical_operation_id operation_id = {};
	bool has_desired = false;
	bool dirty = false;
	bool pending = false;
	bool fenced = false;
	corpse_lifecycle_release_completion_fn release_completion = nullptr;
	corpse_lifecycle_release_completion_fn queued_destruction_completion = nullptr;
};

enum class submit_outcome
{
	submitted,
	deferred,
	failed,
};

std::unordered_map<uint64_t, corpse_state> states;
std::unordered_map<std::string, uint64_t> operations;
corpse_lifecycle_transaction_health health = {};

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

uint64_t corpse_key(uint32_t owner_pid, uint32_t save_id)
{
	return item_corpse_owner_id(owner_pid, save_id);
}

bool valid_staged_payload(const corpse_lifecycle_payload &payload)
{
	if (payload.expected_corpse_revision ||
	    payload.action == corpse_lifecycle_action::release ||
	    payload.action == corpse_lifecycle_action::destroy ||
	    payload.action == corpse_lifecycle_action::resurrect ||
	    payload.action == corpse_lifecycle_action::raise_follower ||
	    payload.action == corpse_lifecycle_action::release_nested)
		return false;
	corpse_lifecycle_payload candidate = payload;
	if (candidate.action == corpse_lifecycle_action::remove)
		candidate.expected_corpse_revision = 1;
	std::vector<uint8_t> ignored;
	return corpse_lifecycle_command_encode_payload(candidate, &ignored);
}

bool valid_release_payload(const corpse_lifecycle_payload &payload)
{
	if ((payload.action != corpse_lifecycle_action::release &&
	     payload.action != corpse_lifecycle_action::release_nested) ||
	    payload.expected_corpse_revision)
		return false;
	corpse_lifecycle_payload candidate = payload;
	candidate.expected_corpse_revision = 1;
	std::vector<uint8_t> ignored;
	return corpse_lifecycle_command_encode_payload(candidate, &ignored);
}

bool valid_destroy_payload(const corpse_lifecycle_payload &payload)
{
	if (payload.action != corpse_lifecycle_action::destroy || payload.expected_corpse_revision)
		return false;
	corpse_lifecycle_payload candidate = payload;
	candidate.expected_corpse_revision = 1;
	std::vector<uint8_t> ignored;
	return corpse_lifecycle_command_encode_payload(candidate, &ignored);
}

bool valid_resurrect_payload(const corpse_lifecycle_payload &payload)
{
	if (payload.action != corpse_lifecycle_action::resurrect ||
	    payload.expected_corpse_revision)
		return false;
	corpse_lifecycle_payload candidate = payload;
	candidate.expected_corpse_revision = 1;
	std::vector<uint8_t> ignored;
	return corpse_lifecycle_command_encode_payload(candidate, &ignored);
}

bool valid_raise_follower_payload(const corpse_lifecycle_payload &payload)
{
	if (payload.action != corpse_lifecycle_action::raise_follower ||
	    payload.expected_corpse_revision)
		return false;
	corpse_lifecycle_payload candidate = payload;
	candidate.expected_corpse_revision = 1;
	std::vector<uint8_t> ignored;
	return corpse_lifecycle_command_encode_payload(candidate, &ignored);
}

void account_health()
{
	health.tracked = states.size();
	health.pending = 0;
	health.dirty = 0;
	health.fenced = 0;
	for (const auto &[key, state] : states)
	{
		(void)key;
		health.pending += state.pending ? 1 : 0;
		health.dirty += state.dirty ? 1 : 0;
		health.fenced += state.fenced ? 1 : 0;
	}
}

submit_outcome submit(uint64_t key, corpse_state *state)
{
	if (!state || state->pending || !state->dirty || state->fenced || !state->has_desired)
		return submit_outcome::deferred;
	if ((state->desired.action == corpse_lifecycle_action::remove ||
	     state->desired.action == corpse_lifecycle_action::release ||
	     state->desired.action == corpse_lifecycle_action::destroy ||
	     state->desired.action == corpse_lifecycle_action::resurrect ||
	     state->desired.action == corpse_lifecycle_action::raise_follower ||
	     state->desired.action == corpse_lifecycle_action::release_nested) &&
	    !state->revision)
	{
		state->dirty = false;
		state->has_desired = false;
		return submit_outcome::deferred;
	}
	const critical_entity_key entity = { critical_entity_type::corpse, key };
	critical_operation_id blocking = {};
	if (critical_command_coordinator_is_fenced(entity, &blocking))
		return submit_outcome::deferred;
	if (state->desired.action == corpse_lifecycle_action::release ||
	    state->desired.action == corpse_lifecycle_action::destroy ||
	    state->desired.action == corpse_lifecycle_action::resurrect ||
	    state->desired.action == corpse_lifecycle_action::raise_follower ||
	    state->desired.action == corpse_lifecycle_action::release_nested)
	{
		critical_entity_key destination = {};
		if (state->desired.action == corpse_lifecycle_action::release)
			destination = { critical_entity_type::room,
					static_cast<uint64_t>(state->desired.room_vnum) };
		else if (state->desired.action == corpse_lifecycle_action::destroy &&
			 !item_owner_key({ item_owner_type::destruction, 0, 0 }, &destination))
			return submit_outcome::failed;
		if (state->desired.action == corpse_lifecycle_action::resurrect)
		{
			const critical_entity_key room = { critical_entity_type::room,
							   static_cast<uint64_t>(
								   state->desired.old_room_vnum) };
			const critical_entity_key player = {
				critical_entity_type::player,
				static_cast<uint64_t>(state->desired.destination_player_pid)
			};
			if (critical_command_coordinator_is_fenced(room, &blocking) ||
			    critical_command_coordinator_is_fenced(player, &blocking))
				return submit_outcome::deferred;
		}
		else if (state->desired.action == corpse_lifecycle_action::raise_follower ||
			 state->desired.action == corpse_lifecycle_action::release_nested)
		{
			if (state->desired.destination_player_pid)
			{
				const critical_entity_key player = {
					critical_entity_type::player,
					static_cast<uint64_t>(state->desired.destination_player_pid)
				};
				if (critical_command_coordinator_is_fenced(player, &blocking))
					return submit_outcome::deferred;
			}
			else
			{
				const critical_entity_key room = {
					critical_entity_type::room,
					static_cast<uint64_t>(state->desired.room_vnum)
				};
				if (critical_command_coordinator_is_fenced(room, &blocking))
					return submit_outcome::deferred;
			}
		}
		else if (critical_command_coordinator_is_fenced(destination, &blocking))
			return submit_outcome::deferred;
	}

	corpse_lifecycle_payload payload = state->desired;
	payload.expected_corpse_revision = state->revision;
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !corpse_lifecycle_command_build(&command, operation_id, payload,
					    critical_source_site::command,
					    critical_deadline_class::terminal))
	{
		state->fenced = true;
		++health.submission_failures;
		return submit_outcome::failed;
	}
	const std::string operation = operation_key(operation_id);
	try
	{
		if (!operations.emplace(operation, key).second)
		{
			state->fenced = true;
			++health.submission_failures;
			return submit_outcome::failed;
		}
	}
	catch (const std::bad_alloc &)
	{
		++health.submission_failures;
		return submit_outcome::failed;
	}
	const auto submitted = critical_command_coordinator_submit(std::move(command));
	if (submitted != critical_submit_result::accepted &&
	    submitted != critical_submit_result::attached)
	{
		operations.erase(operation);
		++health.submission_failures;
		return submit_outcome::failed;
	}
	state->operation_id = operation_id;
	state->inflight = std::move(payload);
	state->pending = true;
	state->dirty = false;
	state->has_desired = false;
	if (state->inflight.action == corpse_lifecycle_action::destroy)
	{
		state->release_completion = state->queued_destruction_completion;
		state->queued_destruction_completion = nullptr;
	}
	++health.submitted;
	return submit_outcome::submitted;
}
} // namespace

bool corpse_lifecycle_transaction_stage(const corpse_lifecycle_payload &payload)
{
	if (!valid_staged_payload(payload))
		return false;
	const uint64_t key = corpse_key(payload.owner_pid, payload.save_id);
	if (!key)
		return false;
	auto found = states.find(key);
	if (found != states.end() &&
	    (found->second.owner_pid != payload.owner_pid ||
	     found->second.save_id != payload.save_id || found->second.fenced ||
	     found->second.release_completion || found->second.queued_destruction_completion ||
	     (found->second.pending &&
	      (found->second.inflight.action == corpse_lifecycle_action::release ||
	       found->second.inflight.action == corpse_lifecycle_action::destroy ||
	       found->second.inflight.action == corpse_lifecycle_action::resurrect ||
	       found->second.inflight.action == corpse_lifecycle_action::raise_follower ||
	       found->second.inflight.action == corpse_lifecycle_action::release_nested))))
		return false;
	if (found == states.end() && states.size() >= CORPSE_LIFECYCLE_PENDING_MAX)
		return false;
	try
	{
		if (found == states.end())
		{
			corpse_state created;
			created.owner_pid = payload.owner_pid;
			created.save_id = payload.save_id;
			found = states.emplace(key, std::move(created)).first;
		}
		found->second.desired = payload;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	corpse_state &state = found->second;
	state.has_desired = true;
	state.dirty = true;
	if (payload.action == corpse_lifecycle_action::upsert && !state.pending)
		(void)submit(key, &state);
	account_health();
	return true;
}

bool corpse_lifecycle_transaction_destroy(const corpse_lifecycle_payload &payload,
					  corpse_lifecycle_release_completion_fn completion)
{
	if (!completion || !valid_destroy_payload(payload))
		return false;
	const uint64_t key = corpse_key(payload.owner_pid, payload.save_id);
	auto found = states.find(key);
	if (!key || found == states.end())
		return false;
	corpse_state &state = found->second;
	if ((state.queued_destruction_completion == completion && state.dirty &&
	     state.has_desired && state.desired.action == corpse_lifecycle_action::destroy) ||
	    (state.release_completion == completion && state.pending &&
	     state.inflight.action == corpse_lifecycle_action::destroy))
		return true;
	if (state.owner_pid != payload.owner_pid || state.save_id != payload.save_id ||
	    state.fenced || state.release_completion || state.queued_destruction_completion ||
	    (!state.revision && !state.pending) ||
	    (state.pending && state.inflight.action != corpse_lifecycle_action::upsert) ||
	    (state.dirty && state.has_desired &&
	     state.desired.action != corpse_lifecycle_action::upsert))
		return false;
	try
	{
		state.desired = payload;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	state.has_desired = true;
	state.dirty = true;
	state.queued_destruction_completion = completion;
	const auto outcome = submit(key, &state);
	if (outcome == submit_outcome::failed)
	{
		state.has_desired = false;
		state.dirty = false;
		state.queued_destruction_completion = nullptr;
		account_health();
		return false;
	}
	account_health();
	return true;
}

bool corpse_lifecycle_transaction_release(const corpse_lifecycle_payload &payload,
					  corpse_lifecycle_release_completion_fn completion)
{
	if (!completion || !valid_release_payload(payload))
		return false;
	const uint64_t key = corpse_key(payload.owner_pid, payload.save_id);
	auto found = states.find(key);
	if (!key || found == states.end())
		return false;
	corpse_state &state = found->second;
	if (state.owner_pid != payload.owner_pid || state.save_id != payload.save_id ||
	    !state.revision || state.pending || state.dirty || state.has_desired || state.fenced ||
	    state.release_completion)
		return false;
	try
	{
		state.desired = payload;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	state.has_desired = true;
	state.dirty = true;
	state.release_completion = completion;
	const auto outcome = submit(key, &state);
	if (outcome != submit_outcome::submitted)
	{
		state.has_desired = false;
		state.dirty = false;
		state.release_completion = nullptr;
		account_health();
		return false;
	}
	account_health();
	return true;
}

bool corpse_lifecycle_transaction_resurrect(const corpse_lifecycle_payload &payload,
					    corpse_lifecycle_release_completion_fn completion)
{
	if (!completion || !valid_resurrect_payload(payload))
		return false;
	const uint64_t key = corpse_key(payload.owner_pid, payload.save_id);
	auto found = states.find(key);
	if (!key || found == states.end())
		return false;
	corpse_state &state = found->second;
	if (state.owner_pid != payload.owner_pid || state.save_id != payload.save_id ||
	    !state.revision || state.pending || state.dirty || state.has_desired || state.fenced ||
	    state.release_completion)
		return false;
	try
	{
		state.desired = payload;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	state.has_desired = true;
	state.dirty = true;
	state.release_completion = completion;
	const auto outcome = submit(key, &state);
	if (outcome != submit_outcome::submitted)
	{
		state.has_desired = false;
		state.dirty = false;
		state.release_completion = nullptr;
		account_health();
		return false;
	}
	account_health();
	return true;
}

bool corpse_lifecycle_transaction_raise_follower(const corpse_lifecycle_payload &payload,
						 corpse_lifecycle_release_completion_fn completion)
{
	if (!completion || !valid_raise_follower_payload(payload))
		return false;
	const uint64_t key = corpse_key(payload.owner_pid, payload.save_id);
	auto found = states.find(key);
	if (!key || found == states.end())
		return false;
	corpse_state &state = found->second;
	if (state.owner_pid != payload.owner_pid || state.save_id != payload.save_id ||
	    !state.revision || state.pending || state.dirty || state.has_desired || state.fenced ||
	    state.release_completion)
		return false;
	try
	{
		state.desired = payload;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	state.has_desired = true;
	state.dirty = true;
	state.release_completion = completion;
	const auto outcome = submit(key, &state);
	if (outcome != submit_outcome::submitted)
	{
		state.has_desired = false;
		state.dirty = false;
		state.release_completion = nullptr;
		account_health();
		return false;
	}
	account_health();
	return true;
}

bool corpse_lifecycle_transaction_hydrate(uint32_t owner_pid, uint32_t save_id,
					  uint64_t corpse_revision)
{
	const uint64_t key = corpse_key(owner_pid, save_id);
	if (!key || !corpse_revision)
		return false;
	auto found = states.find(key);
	if (found == states.end() && states.size() >= CORPSE_LIFECYCLE_PENDING_MAX)
		return false;
	try
	{
		if (found == states.end())
		{
			corpse_state created;
			created.owner_pid = owner_pid;
			created.save_id = save_id;
			created.revision = corpse_revision;
			found = states.emplace(key, std::move(created)).first;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	corpse_state &state = found->second;
	if (state.owner_pid != owner_pid || state.save_id != save_id || state.pending ||
	    state.dirty || (state.revision && state.revision != corpse_revision))
		return false;
	state.revision = corpse_revision;
	state.fenced = false;
	account_health();
	return true;
}

bool corpse_lifecycle_transaction_note_item_transfer(uint32_t owner_pid, uint32_t save_id,
						     uint64_t corpse_revision)
{
	const uint64_t key = corpse_key(owner_pid, save_id);
	if (!key || !corpse_revision)
		return false;
	auto found = states.find(key);
	if (found == states.end() && states.size() >= CORPSE_LIFECYCLE_PENDING_MAX)
		return false;
	try
	{
		if (found == states.end())
		{
			corpse_state created;
			created.owner_pid = owner_pid;
			created.save_id = save_id;
			created.revision = corpse_revision;
			found = states.emplace(key, std::move(created)).first;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	corpse_state &state = found->second;
	if (state.owner_pid != owner_pid || state.save_id != save_id ||
	    corpse_revision < state.revision)
		return false;
	const bool advanced = corpse_revision > state.revision;
	state.revision = corpse_revision;
	if (advanced)
		state.fenced = false;
	if (state.dirty && !state.pending)
		(void)submit(key, &state);
	account_health();
	return true;
}

bool corpse_lifecycle_transaction_forget(uint32_t owner_pid, uint32_t save_id)
{
	const uint64_t key = corpse_key(owner_pid, save_id);
	auto found = states.find(key);
	if (found == states.end())
		return true;
	if (found->second.pending)
		return false;
	states.erase(found);
	account_health();
	return true;
}

void corpse_lifecycle_transaction_pulse(void)
{
	for (auto &[key, state] : states)
		if (state.dirty && !state.pending && !state.fenced)
			(void)submit(key, &state);
	account_health();
}

void corpse_lifecycle_transaction_handle_completions(const critical_completion *completions,
						     size_t count)
{
	if (count && !completions)
		return;
	for (size_t index = 0; index < count; ++index)
	{
		const std::string operation = operation_key(completions[index].operation_id);
		auto owner = operations.find(operation);
		if (owner == operations.end())
			continue;
		auto found = states.find(owner->second);
		operations.erase(owner);
		if (found == states.end() || !found->second.pending ||
		    !critical_operation_id_equal(found->second.operation_id,
						 completions[index].operation_id))
			continue;
		corpse_state &state = found->second;
		state.pending = false;
		corpse_lifecycle_result result = {};
		const bool decoded = corpse_lifecycle_command_decode_result(
			completions[index].result_payload.data(), completions[index].result_size,
			&result);
		const bool committed =
			decoded &&
			(completions[index].outcome == critical_apply_outcome::applied ||
			 completions[index].outcome == critical_apply_outcome::already_applied) &&
			result.owner_pid == state.owner_pid && result.save_id == state.save_id &&
			result.action == state.inflight.action;
		const unsigned int error_code = decoded || completions[index].error_code ?
							completions[index].error_code :
							EBADMSG;
		const corpse_lifecycle_payload inflight = state.inflight;
		const corpse_lifecycle_release_completion_fn release_completion =
			state.release_completion;
		corpse_lifecycle_release_completion_fn queued_failure = nullptr;
		corpse_lifecycle_payload queued_failure_payload = {};
		state.release_completion = nullptr;
		if (committed)
		{
			state.revision = result.corpse_revision;
			++health.committed;
		}
		else
		{
			++health.rejected;
			if (state.inflight.action == corpse_lifecycle_action::remove &&
			    completions[index].error_code == ENOENT)
				state.revision = 0;
			else if ((state.inflight.action == corpse_lifecycle_action::release ||
				  state.inflight.action == corpse_lifecycle_action::destroy ||
				  state.inflight.action == corpse_lifecycle_action::resurrect ||
				  state.inflight.action ==
					  corpse_lifecycle_action::raise_follower ||
				  state.inflight.action ==
					  corpse_lifecycle_action::release_nested) &&
				 completions[index].error_code == ESTALE)
				state.fenced = false;
			else if (!(state.inflight.action == corpse_lifecycle_action::remove &&
				   completions[index].error_code == ENOTEMPTY))
				state.fenced = true;
		}
		if (state.dirty && !state.fenced)
			(void)submit(found->first, &state);
		if (state.queued_destruction_completion && state.fenced)
		{
			queued_failure = state.queued_destruction_completion;
			queued_failure_payload = state.desired;
			state.queued_destruction_completion = nullptr;
			state.dirty = false;
			state.has_desired = false;
		}
		if (!state.pending && !state.dirty && !state.revision && !state.has_desired)
			states.erase(found);
		if (release_completion)
			release_completion(committed, decoded ? result : corpse_lifecycle_result{},
					   error_code, inflight);
		if (queued_failure)
			queued_failure(false, {}, error_code, queued_failure_payload);
	}
	account_health();
}

bool corpse_lifecycle_transaction_busy(uint32_t owner_pid, uint32_t save_id)
{
	const auto found = states.find(corpse_key(owner_pid, save_id));
	return found != states.end() && (found->second.pending || found->second.dirty);
}

corpse_lifecycle_transaction_health corpse_lifecycle_transaction_health_copy(void)
{
	account_health();
	return health;
}

void corpse_lifecycle_transaction_reset_for_tests(void)
{
	states.clear();
	operations.clear();
	health = {};
}
